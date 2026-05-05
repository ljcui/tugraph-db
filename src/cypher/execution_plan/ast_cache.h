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

#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "cypher/parser/data_typedef.h"
#include "geax-front-end/ast/AstNode.h"
#include "geax-front-end/common/ObjectAllocator.h"

namespace cypher {

struct CachedAst {
  geax::common::ObjectArenaAllocator obj_alloc;
  geax::frontend::AstNode* ast = nullptr;
  parser::CmdType command_type = parser::CmdType::QUERY;
};

class AstCache {
 public:
  static AstCache& Instance();

  std::shared_ptr<CachedAst> Get(const std::string& cypher);
  void Put(const std::string& cypher, std::shared_ptr<CachedAst> ast);
  void Clear();

 private:
  struct Entry {
    std::string key;
    std::shared_ptr<CachedAst> value;
  };

  static constexpr size_t kMaxEntries = 1024;

  std::mutex mutex_;
  std::list<Entry> lru_;
  std::unordered_map<std::string, std::list<Entry>::iterator> index_;
};

std::shared_ptr<CachedAst> ParseAndRewriteAst(const std::string& cypher);

}  // namespace cypher
