/**
 * Copyright 2026 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#include "cypher/execution_plan/ast_cache.h"

#include <antlr4-runtime/antlr4-runtime.h>

#include "cypher/parser/cypher_base_visitor_v2.h"
#include "cypher/parser/cypher_error_listener.h"
#include "cypher/parser/generated/LcypherLexer.h"
#include "cypher/parser/generated/LcypherParser.h"
#include "cypher/rewriter/GenAnonymousAliasRewriter.h"
#include "cypher/rewriter/MultiPathPatternRewriter.h"
#include "cypher/rewriter/StandaloneCallYieldRewriter.h"

namespace cypher {

AstCache& AstCache::Instance() {
  static AstCache cache;
  return cache;
}

std::shared_ptr<CachedAst> AstCache::Get(const std::string& cypher) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = index_.find(cypher);
  if (iter == index_.end()) {
    return nullptr;
  }
  lru_.splice(lru_.begin(), lru_, iter->second);
  return iter->second->value;
}

void AstCache::Put(const std::string& cypher, std::shared_ptr<CachedAst> ast) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = index_.find(cypher);
  if (iter != index_.end()) {
    iter->second->value = std::move(ast);
    lru_.splice(lru_.begin(), lru_, iter->second);
    return;
  }

  lru_.push_front({cypher, std::move(ast)});
  index_.emplace(lru_.front().key, lru_.begin());
  if (lru_.size() > kMaxEntries) {
    index_.erase(lru_.back().key);
    lru_.pop_back();
  }
}

void AstCache::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  index_.clear();
  lru_.clear();
}

std::shared_ptr<CachedAst> ParseAndRewriteAst(const std::string& cypher) {
  antlr4::ANTLRInputStream input(cypher);
  parser::LcypherLexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  parser::LcypherParser parser(&tokens);
  parser.addErrorListener(&parser::CypherErrorListener::INSTANCE);

  auto cached = std::make_shared<CachedAst>();
  parser::CypherBaseVisitorV2 visitor(cached->obj_alloc, parser.oC_Cypher(),
                                      nullptr);
  cached->ast = visitor.result();
  cached->command_type = visitor.CommandType();

  cypher::StandaloneCallYieldRewriter standalone_call_yield_rewriter(
      cached->obj_alloc);
  cached->ast->accept(standalone_call_yield_rewriter);
  cypher::GenAnonymousAliasRewriter gen_anonymous_alias_rewriter;
  cached->ast->accept(gen_anonymous_alias_rewriter);
  cypher::MultiPathPatternRewriter multi_path_pattern_rewriter(
      cached->obj_alloc);
  cached->ast->accept(multi_path_pattern_rewriter);

  return cached;
}

}  // namespace cypher
