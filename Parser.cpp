//
// Parser is used to invoke the clang libraries to perform actual parsing of
// the input received in the Console.
//
// Part of ccons, the interactive console for the C programming language.
//
// Copyright (c) 2009 Alexei Svitkine. This file is distributed under the
// terms of MIT Open Source License. See file LICENSE for details.
//

#include "Parser.h"

#include <iostream>
#include <stack>
#include <algorithm>

#include <llvm/Config/config.h>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/HeaderSearchOptions.h>
#include <clang/Frontend/PreprocessorOptions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/LexDiagnostic.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Sema/SemaDiagnostic.h>

#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/Diagnostic.h>

#include "Diagnostics.h"
#include "SrcGen.h"

using std::string;

namespace ccons {


//
// ParseOperation
//

ParseOperation::ParseOperation(const clang::LangOptions& options,
                               clang::Diagnostic *diag,
                               clang::PPCallbacks *callbacks) :
	_fsOpts(new clang::FileSystemOptions),
	_fm(new clang::FileManager(*_fsOpts)),
	_sm(new clang::SourceManager(*diag, *_fm)),
	_hs(new clang::HeaderSearch(*_fm))
{
	llvm::Triple triple(LLVM_HOSTTRIPLE);
	clang::TargetOptions targetOptions;
	targetOptions.ABI = "";
	targetOptions.CPU = "";
	targetOptions.Features.clear();
	targetOptions.Triple = LLVM_HOSTTRIPLE;
	_target.reset(clang::TargetInfo::CreateTargetInfo(*diag, targetOptions));
	clang::HeaderSearchOptions hsOptions;
	ApplyHeaderSearchOptions(*_hs, hsOptions, options, triple);
	_pp.reset(new clang::Preprocessor(*diag, options, *_target, *_sm, *_hs));
	_pp->addPPCallbacks(callbacks);
	clang::PreprocessorOptions ppOptions;
	clang::FrontendOptions frontendOptions;
	InitializePreprocessor(*_pp, ppOptions, hsOptions, frontendOptions);
	_ast.reset(new clang::ASTContext(options,
	                                 *_sm,
	                                 *_target,
	                                 _pp->getIdentifierTable(),
	                                 _pp->getSelectorTable(),
	                                 _pp->getBuiltinInfo(),
	                                 0));
}

clang::ASTContext * ParseOperation::getASTContext() const
{
	return _ast.get();
}

clang::Preprocessor * ParseOperation::getPreprocessor() const
{
	return _pp.get();
}

clang::SourceManager * ParseOperation::getSourceManager() const
{
	return _sm.get();
}

clang::TargetInfo * ParseOperation::getTargetInfo() const
{
	return _target.get();
}


//
// Parser
//



Parser::Parser(const clang::LangOptions& options) :
	_options(options)
{
}

Parser::~Parser()
{
	releaseAccumulatedParseOperations();
}

void Parser::releaseAccumulatedParseOperations()
{
	for (std::vector<ParseOperation*>::iterator I = _ops.begin(), E = _ops.end();
	     I != E; ++I) {
		delete *I;
	}
	_ops.clear();
}


ParseOperation * Parser::getLastParseOperation() const
{
	return _ops.empty() ? NULL : _ops.back();
}

Parser::InputType Parser::analyzeInput(const string& contextSource,
                                       const string& buffer,
                                       int& indentLevel,
                                       std::vector<clang::FunctionDecl*> *fds)
{
	if (buffer.length() > 1 && buffer[buffer.length() - 2] == '\\') {
		indentLevel = 1;
		return Incomplete;
	}

	NullDiagnosticProvider ndp;
	llvm::OwningPtr<ParseOperation>
		parseOp(new ParseOperation(_options, ndp.getDiagnostic()));
	llvm::MemoryBuffer *memBuf =
		createMemoryBuffer(buffer, "", parseOp->getSourceManager());

	clang::Token LastTok;
	bool TokWasDo = false;
	int stackSize =
		analyzeTokens(*parseOp->getPreprocessor(), memBuf, LastTok, indentLevel, TokWasDo);
	if (stackSize < 0)
		return TopLevel;

	// TokWasDo is used for do { ... } while (...); loops
	if (LastTok.is(clang::tok::semi) || (LastTok.is(clang::tok::r_brace) && !TokWasDo)) {
		if (stackSize > 0) return Incomplete;
		NullDiagnosticProvider ndp;
		clang::Diagnostic& diag = *ndp.getDiagnostic();
		// Setting this ensures "foo();" is not a valid top-level declaration.
		diag.setDiagnosticMapping(clang::diag::ext_missing_type_specifier,
		                          clang::diag::MAP_ERROR, clang::SourceLocation());
		diag.setSuppressSystemWarnings(true);
		string src = contextSource + buffer;
		struct : public clang::ASTConsumer {
			bool hadIncludedDecls;
			unsigned pos;
			unsigned maxPos;
			clang::SourceManager *sm;
			std::vector<clang::FunctionDecl*> fds;
			void HandleTopLevelDecl(clang::DeclGroupRef D) {
				for (clang::DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; ++I) {
					if (clang::FunctionDecl *FD = llvm::dyn_cast<clang::FunctionDecl>(*I)) {
						clang::SourceLocation Loc = FD->getTypeSpecStartLoc();
						if (!Loc.isValid())
							continue;
						if (sm->isFromMainFile(Loc)) {
							unsigned offset = sm->getFileOffset(sm->getExpansionLoc(Loc));
							if (offset >= pos) {
								fds.push_back(FD);
							}
						} else {
							while (!sm->isFromMainFile(Loc)) {
								const clang::SrcMgr::SLocEntry& Entry =
									sm->getSLocEntry(sm->getFileID(sm->getSpellingLoc(Loc)));
								if (!Entry.isFile())
									break;
								Loc = Entry.getFile().getIncludeLoc();
							}
							unsigned offset = sm->getFileOffset(Loc);
							if (offset >= pos) {
								hadIncludedDecls = true;
							}
						}
					}
				}
			}
		} consumer;
		ParseOperation *parseOp = createParseOperation(&diag);
		consumer.hadIncludedDecls = false;
		consumer.pos = contextSource.length();
		consumer.maxPos = consumer.pos + buffer.length();
		consumer.sm = parseOp->getSourceManager();
		parse(src, parseOp, &consumer);
		ProxyDiagnosticClient *pdc = ndp.getProxyDiagnosticClient();
		if (pdc->hadError(clang::diag::err_unterminated_block_comment))
			return Incomplete;
		if (!pdc->hadErrors() && (!consumer.fds.empty() || consumer.hadIncludedDecls)) {
			if (!consumer.fds.empty())
				fds->swap(consumer.fds);
			return TopLevel;
		}
		return Stmt;
	}

	return Incomplete;
}

int Parser::analyzeTokens(clang::Preprocessor& PP,
                          const llvm::MemoryBuffer *MemBuf,
                          clang::Token& LastTok,
                          int& IndentLevel,
                          bool& TokWasDo)
{
	int result;
	std::stack<std::pair<clang::Token, clang::Token> > S; // Tok, PrevTok

	IndentLevel = 0;
	PP.EnterMainSourceFile();

	clang::Token Tok;
	PP.Lex(Tok);
	while (Tok.isNot(clang::tok::eof)) {
		if (Tok.is(clang::tok::l_square)) {
			S.push(std::make_pair(Tok, LastTok)); // [
		} else if (Tok.is(clang::tok::l_paren)) {
			S.push(std::make_pair(Tok, LastTok)); // (
		} else if (Tok.is(clang::tok::l_brace)) {
			S.push(std::make_pair(Tok, LastTok)); // {
			IndentLevel++;
		} else if (Tok.is(clang::tok::r_square)) {
			if (S.empty() || S.top().first.isNot(clang::tok::l_square)) {
				std::cout << "Unmatched [\n";
				return -1;
			}
			TokWasDo = false;
			S.pop();
		} else if (Tok.is(clang::tok::r_paren)) {
			if (S.empty() || S.top().first.isNot(clang::tok::l_paren)) {
				std::cout << "Unmatched (\n";
				return -1;
			}
			TokWasDo = false;
			S.pop();
		} else if (Tok.is(clang::tok::r_brace)) {
			if (S.empty() || S.top().first.isNot(clang::tok::l_brace)) {
				std::cout << "Unmatched {\n";
				return -1;
			}
			TokWasDo = S.top().second.is(clang::tok::kw_do);
			S.pop();
			IndentLevel--;
		}
		LastTok = Tok;
		PP.Lex(Tok);
	}
	result = S.size();

	// TODO: We need to properly account for indent-level for blocks that do not
	//       have braces... such as:
	//
	//       if (X)
	//         Y;
	//
	// TODO: Do-while without braces doesn't work, e.g.:
	//
	//       do
	//         foo();
	//       while (bar());
	//
	// Both of the above could be solved by some kind of rewriter-pass that would
	// insert implicit braces (or simply a more involved analysis).

	// Also try to match preprocessor conditionals...
	if (result == 0) {
		clang::Lexer Lexer(PP.getSourceManager().getMainFileID(),
		                   MemBuf,
		                   PP.getSourceManager(),
		                   PP.getLangOptions());
		Lexer.LexFromRawLexer(Tok);
		while (Tok.isNot(clang::tok::eof)) {
			if (Tok.is(clang::tok::hash)) {
				Lexer.LexFromRawLexer(Tok);
				if (clang::IdentifierInfo *II = PP.LookUpIdentifierInfo(Tok)) { 
					switch (II->getPPKeywordID()) {
						case clang::tok::pp_if:
						case clang::tok::pp_ifdef:
						case clang::tok::pp_ifndef:
							result++;
							break;
						case clang::tok::pp_endif:
							if (result == 0)
								return -1; // Nesting error.
							result--;
							break;
						default:
							break;
					}
				}
			}
			Lexer.LexFromRawLexer(Tok);
		}
	}

	return result;
}

ParseOperation * Parser::createParseOperation(clang::Diagnostic *diag,
                                              clang::PPCallbacks *callbacks)
{
	return new ParseOperation(_options, diag, callbacks);
}

void Parser::parse(const string& src,
                   ParseOperation *parseOp,
                   clang::ASTConsumer *consumer)
{
	_ops.push_back(parseOp);
	createMemoryBuffer(src, "", parseOp->getSourceManager());
	clang::ParseAST(*parseOp->getPreprocessor(), consumer,
	                *parseOp->getASTContext());
}


void Parser::parse(const string& src,
                   clang::Diagnostic *diag,
                   clang::ASTConsumer *consumer)
{
	parse(src, createParseOperation(diag), consumer);
}


llvm::MemoryBuffer * Parser::createMemoryBuffer(const string& src,
                                                const char *name,
                                                clang::SourceManager *sm)
{
	llvm::MemoryBuffer *mb =
		llvm::MemoryBuffer::getMemBufferCopy(src, name);
	assert(mb && "Error creating MemoryBuffer!");
	sm->createMainFileIDForMemBuffer(mb);
	assert(!sm->getMainFileID().isInvalid() && "Error creating MainFileID!");
	return mb;
}

} // namespace ccons
