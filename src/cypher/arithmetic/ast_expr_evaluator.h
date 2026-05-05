/**
 * Copyright 2023 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <rocksdb/slice.h>

#include <any>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cypher/arithmetic/agg_ctx.h"
#include "cypher/arithmetic/ast_agg_expr_detector.h"
#include "cypher/cypher_types.h"
#include "cypher/execution_plan/visitor/visitor.h"
#include "cypher/parser/symbol_table.h"
#include "cypher/resultset/record.h"
#include "geax-front-end/ast/Ast.h"
#include "geax-front-end/ast/expr/AggFunc.h"

namespace cypher {

class AstExprEvaluator : public geax::frontend::AstExprNodeVisitorImpl {
 public:
  AstExprEvaluator() = delete;

  AstExprEvaluator(geax::frontend::Expr* expr, const SymbolTable* sym_tab)
      : expr_(expr), sym_tab_(sym_tab) {}

  ~AstExprEvaluator() = default;

  std::vector<geax::frontend::Expr*> agg_exprs_;
  std::vector<std::shared_ptr<AggCtx>> agg_ctxs_;
  size_t agg_pos_;

  enum class VisitMode {
    EVALUATE,
    AGGREGATE,
  } visit_mode_;

  Entry Evaluate(RTContext* ctx, const Record* record) {
    ctx_ = ctx;
    record_ = record;
    agg_pos_ = 0;
    visit_mode_ = VisitMode::EVALUATE;
    Entry entry;
    entry = std::any_cast<Entry>(expr_->accept(*this));
    return entry;
  }

  void Aggregate(RTContext* ctx, const Record* record) {
    ctx_ = ctx;
    record_ = record;
    visit_mode_ = VisitMode::AGGREGATE;
    if (agg_exprs_.empty()) {
      agg_exprs_ = AstAggExprDetector::GetAggExprs(expr_);
    }
    for (size_t i = 0; i < agg_exprs_.size(); i++) {
      agg_pos_ = i;
      agg_exprs_[i]->accept(*this);
    }
  }

  void Reduce() {
    for (auto agg_ctx : agg_ctxs_) {
      agg_ctx->ReduceNext();
    }
  }

 private:
  std::any visit(geax::frontend::GetField* node) override;
  std::any visit(geax::frontend::TupleGet* node) override;
  std::any visit(geax::frontend::Not* node) override;
  std::any visit(geax::frontend::Neg* node) override;
  std::any visit(geax::frontend::Tilde* node) override;
  std::any visit(geax::frontend::VSome* node) override;
  std::any visit(geax::frontend::BEqual* node) override;
  std::any visit(geax::frontend::BNotEqual* node) override;
  std::any visit(geax::frontend::BGreaterThan* node) override;
  std::any visit(geax::frontend::BNotSmallerThan* node) override;
  std::any visit(geax::frontend::BSmallerThan* node) override;
  std::any visit(geax::frontend::BNotGreaterThan* node) override;
  std::any visit(geax::frontend::BSafeEqual* node) override;
  std::any visit(geax::frontend::BAdd* node) override;
  std::any visit(geax::frontend::BSub* node) override;
  std::any visit(geax::frontend::BDiv* node) override;
  std::any visit(geax::frontend::BMul* node) override;
  std::any visit(geax::frontend::BMod* node) override;
  std::any visit(geax::frontend::BSquare* node) override;
  std::any visit(geax::frontend::BAnd* node) override;
  std::any visit(geax::frontend::BOr* node) override;
  std::any visit(geax::frontend::BXor* node) override;
  std::any visit(geax::frontend::BBitAnd* node) override;
  std::any visit(geax::frontend::BBitOr* node) override;
  std::any visit(geax::frontend::BBitXor* node) override;
  std::any visit(geax::frontend::BBitLeftShift* node) override;
  std::any visit(geax::frontend::BBitRightShift* node) override;
  std::any visit(geax::frontend::BConcat* node) override;
  std::any visit(geax::frontend::BIndex* node) override;
  std::any visit(geax::frontend::BLike* node) override;
  std::any visit(geax::frontend::BIn* node) override;
  std::any visit(geax::frontend::If* node) override;
  std::any visit(geax::frontend::Function* node) override;
  std::any visit(geax::frontend::Case* node) override;
  std::any visit(geax::frontend::Cast* node) override;
  std::any visit(geax::frontend::MatchCase* node) override;
  std::any visit(geax::frontend::AggFunc* node) override;
  std::any visit(geax::frontend::BAggFunc* node) override;
  std::any visit(geax::frontend::MultiCount* node) override;
  std::any visit(geax::frontend::Windowing* node) override;
  std::any visit(geax::frontend::MkList* node) override;
  std::any visit(geax::frontend::MkMap* node) override;
  std::any visit(geax::frontend::MkRecord* node) override;
  std::any visit(geax::frontend::MkSet* node) override;
  std::any visit(geax::frontend::MkTuple* node) override;
  std::any visit(geax::frontend::VBool* node) override;
  std::any visit(geax::frontend::VInt* node) override;
  std::any visit(geax::frontend::VDouble* node) override;
  std::any visit(geax::frontend::VString* node) override;
  std::any visit(geax::frontend::VDate* node) override;
  std::any visit(geax::frontend::VDatetime* node) override;
  std::any visit(geax::frontend::VDuration* node) override;
  std::any visit(geax::frontend::VTime* node) override;
  std::any visit(geax::frontend::VNull* node) override;
  std::any visit(geax::frontend::VNone* node) override;
  std::any visit(geax::frontend::Ref* node) override;
  std::any visit(geax::frontend::Param* node) override;
  std::any visit(geax::frontend::SingleLabel* node) override;
  std::any visit(geax::frontend::LabelOr* node) override;
  std::any visit(geax::frontend::IsLabeled* node) override;
  std::any visit(geax::frontend::IsNull* node) override;
  std::any visit(geax::frontend::ListComprehension* node) override;
  std::any visit(geax::frontend::PredicateFunction* node) override;
  std::any visit(geax::frontend::Exists* node) override;

  std::any reportError() override;

 private:
  enum class NumericVectorStorage {
    DOUBLE,
    RAW_FLOAT,
  };

  struct NumericVectorScratch {
    std::vector<double> doubles;
    rocksdb::PinnableSlice raw_floats;
    size_t raw_dimensions = 0;
  };

  struct NumericVectorView {
    NumericVectorStorage storage = NumericVectorStorage::DOUBLE;
    const std::vector<double>* doubles = nullptr;
    const rocksdb::Slice* raw_floats = nullptr;
    size_t raw_dimensions = 0;
    double norm_sq = 0.0;

    size_t Size() const {
      if (storage == NumericVectorStorage::RAW_FLOAT) return raw_dimensions;
      return doubles->size();
    }
  };

  struct CachedNumericVector {
    std::vector<double> values;
    double norm_sq = 0.0;
  };

  bool TryEvaluateVectorSimilarityFunction(geax::frontend::Function* node,
                                           const std::string& func_name,
                                           Entry* result);
  void LoadNumericVector(geax::frontend::Expr* expr,
                         NumericVectorScratch* scratch,
                         NumericVectorView* view);
  bool TryLoadCachedNumericVector(geax::frontend::Expr* expr,
                                  NumericVectorView* view);
  bool TryLoadNodeVectorField(geax::frontend::Expr* expr,
                              NumericVectorScratch* scratch,
                              NumericVectorView* view);

  std::string error_msg_;
  geax::frontend::Expr* expr_;
  RTContext* ctx_;
  const SymbolTable* sym_tab_;
  const Record* record_;
  std::shared_ptr<AggCtx> agg_func_;
  std::unordered_map<geax::frontend::Expr*, CachedNumericVector>
      numeric_vector_cache_;
  std::unordered_map<std::string, uint32_t> property_pid_cache_;
};

}  // namespace cypher
