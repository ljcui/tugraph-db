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

#include "cypher/arithmetic/ast_expr_evaluator.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "common/byte_utils.h"
#include "common/exceptions.h"
#include "cypher/arithmetic/arithmetic_expression.h"
#include "cypher/cypher_types.h"
#include "cypher/resultset/record.h"
#include "cypher/utils/geax_util.h"

#ifndef DO_BINARY_EXPR
#define DO_BINARY_EXPR(func)                                     \
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));  \
  auto rig = std::any_cast<Entry>(node->right()->accept(*this)); \
  if (lef.type != Entry::RecordEntryType::CONSTANT ||            \
      rig.type != Entry::RecordEntryType::CONSTANT) {            \
    NOT_SUPPORT_AND_THROW();                                     \
  }                                                              \
  return Entry(cypher::func(lef.constant, rig.constant));
#endif

#ifndef DO_UNARY_EXPR
#define DO_UNARY_EXPR(func)                                      \
  auto expr = std::any_cast<Entry>(node->expr()->accept(*this)); \
  if (expr.type != Entry::RecordEntryType::CONSTANT) {           \
    NOT_SUPPORT_AND_THROW();                                     \
  }                                                              \
  return Entry(cypher::func(expr.constant));
#endif

Value doCallBuiltinFunc(const std::string& name, cypher::RTContext* ctx,
                        const cypher::Record& record,
                        const std::vector<cypher::ArithExprNode>& args) {
  static std::unordered_map<std::string, cypher::BuiltinFunction::FUNC>
      ae_registered_funcs = cypher::ArithOpNode::RegisterFuncs();
  auto it = ae_registered_funcs.find(name);
  if (it == ae_registered_funcs.end()) NOT_SUPPORT_AND_THROW();
  cypher::BuiltinFunction::FUNC func = it->second;
  auto data = func(ctx, record, args);
  return data;
}

const std::string& getNodeOrEdgeName(geax::frontend::AstNode* ast_node) {
  if (ast_node->type() == geax::frontend::AstNodeType::kNode) {
    geax::frontend::Node* node = (geax::frontend::Node*)ast_node;
    if (!node->filler()->v().has_value()) NOT_SUPPORT_AND_THROW();
    return node->filler()->v().value();
  } else if (ast_node->type() == geax::frontend::AstNodeType::kEdge) {
    geax::frontend::Edge* edge = (geax::frontend::Edge*)ast_node;
    if (!edge->filler()->v().has_value()) NOT_SUPPORT_AND_THROW();
    return edge->filler()->v().value();
  } else {
    NOT_SUPPORT_AND_THROW();
  }
}

namespace cypher {

namespace {

Entry EvalConstantExpr(RTContext* ctx, geax::frontend::Expr* expr) {
  SymbolTable empty_sym_tab;
  Record empty_record(0, &empty_sym_tab);
  AstExprEvaluator evaluator(expr, &empty_sym_tab);
  return evaluator.Evaluate(ctx, &empty_record);
}

double NumericValueAsDouble(const Value& value, const char* function_name) {
  if (value.IsInteger()) {
    return static_cast<double>(value.AsInteger());
  }
  if (value.IsFloat()) {
    return static_cast<double>(value.AsFloat());
  }
  if (value.IsDouble()) {
    return value.AsDouble();
  }
  THROW_CODE(CypherException, "Invalid argument of " +
                                  std::string(function_name) +
                                  ": vector element must be numeric");
}

double RawFloatAt(const rocksdb::Slice& values, size_t i) {
  return static_cast<double>(
      common::ReadValue<float>(values.data() + i * sizeof(float)));
}

}  // namespace

static Value And(const Value& x, const Value& y) {
  Value ret;
  if (x.IsBool() && y.IsBool()) {
    ret = Value(x.AsBool() && y.AsBool());
    return ret;
  }
  if (x.IsNull() && y.IsBool()) {
    return !y.AsBool() ? Value::Bool(false) : Value();
  }
  if (x.IsBool() && y.IsNull()) {
    return !x.AsBool() ? Value::Bool(false) : Value();
  }
  if (x.IsNull() && y.IsNull()) {
    return {};
  }
  THROW_CODE(ParserException, "Type error");
}

static Value Or(const Value& x, const Value& y) {
  Value ret;
  if (x.IsBool() && y.IsBool()) {
    ret = Value(x.AsBool() || y.AsBool());
    return ret;
  }
  if (x.IsNull() && y.IsBool()) {
    return y.AsBool() ? Value::Bool(true) : Value();
  }
  if (x.IsBool() && y.IsNull()) {
    return x.AsBool() ? Value::Bool(true) : Value();
  }
  if (x.IsNull() && y.IsNull()) {
    return {};
  }
  THROW_CODE(ParserException, "Type error");
}

static Value Xor(const Value& x, const Value& y) {
  Value ret;
  if (x.IsBool() && y.IsBool()) {
    ret = Value(!x.AsBool() != !y.AsBool());
    return ret;
  }
  if ((x.IsBool() && y.IsNull()) || (x.IsNull() && y.IsBool()) ||
      (x.IsNull() && y.IsNull())) {
    return {};
  }
  THROW_CODE(ParserException, "Type error");
}

static Value Not(const Value& x) {
  Value ret;
  if (x.IsBool()) {
    ret = Value(!x.AsBool());
    return ret;
  }
  if (x.IsNull()) {
    return {};
  }
  THROW_CODE(ParserException, "Type error");
}

static Value Neg(const Value& x) {
  if (!((IsNumeric(x) || x.IsNull()))) {
    THROW_CODE(CypherException,
               "Type mismatch: expect Integer or Float in sub expr");
  }
  Value ret;
  if (x.IsNull()) return ret;
  if (x.IsInteger()) {
    ret = Value(-x.AsInteger());
    return ret;
  } else {
    ret = Value(-x.AsDouble());
    return ret;
  }
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::GetField* node) {
  auto expr = std::any_cast<Entry>(node->expr()->accept(*this));
  return Entry(expr.GetEntityField(ctx_, node->fieldName()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::TupleGet* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Not* node) {
  DO_UNARY_EXPR(Not);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Neg* node) {
  DO_UNARY_EXPR(Neg);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Tilde* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VSome* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BEqual* node) {
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));
  auto rig = std::any_cast<Entry>(node->right()->accept(*this));
  if (lef.EqualNull() || rig.EqualNull()) {
    return Entry(Value());
  }
  return Entry(Value(lef == rig));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BNotEqual* node) {
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));
  auto rig = std::any_cast<Entry>(node->right()->accept(*this));
  if (lef.EqualNull() || rig.EqualNull()) {
    return Entry(Value());
  }
  return Entry(Value(lef != rig));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BGreaterThan* node) {
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));
  auto rig = std::any_cast<Entry>(node->right()->accept(*this));
  if (lef.type != Entry::RecordEntryType::CONSTANT ||
      rig.type != Entry::RecordEntryType::CONSTANT) {
    NOT_SUPPORT_AND_THROW();
  }
  if (lef.EqualNull() || rig.EqualNull()) {
    return Entry(Value());
  }
  return Entry(Value(lef > rig));
}

std::any cypher::AstExprEvaluator::visit(
    geax::frontend::BNotSmallerThan* node) {
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));
  auto rig = std::any_cast<Entry>(node->right()->accept(*this));
  if (lef.type != Entry::RecordEntryType::CONSTANT ||
      rig.type != Entry::RecordEntryType::CONSTANT) {
    NOT_SUPPORT_AND_THROW();
  }
  if (lef.EqualNull() || rig.EqualNull()) {
    return Entry(Value());
  }
  return Entry(Value(!(lef < rig)));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BSmallerThan* node) {
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));
  auto rig = std::any_cast<Entry>(node->right()->accept(*this));
  if (lef.type != Entry::RecordEntryType::CONSTANT ||
      rig.type != Entry::RecordEntryType::CONSTANT) {
    NOT_SUPPORT_AND_THROW();
  }
  if (lef.EqualNull() || rig.EqualNull()) {
    return Entry(Value());
  }
  return Entry(Value(lef < rig));
}

std::any cypher::AstExprEvaluator::visit(
    geax::frontend::BNotGreaterThan* node) {
  auto lef = std::any_cast<Entry>(node->left()->accept(*this));
  auto rig = std::any_cast<Entry>(node->right()->accept(*this));
  if (lef.type != Entry::RecordEntryType::CONSTANT ||
      rig.type != Entry::RecordEntryType::CONSTANT) {
    NOT_SUPPORT_AND_THROW();
  }
  if (lef.EqualNull() || rig.EqualNull()) {
    return Entry(Value());
  }
  return Entry(Value(!(lef > rig)));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BSafeEqual* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BAdd* node) {
  DO_BINARY_EXPR(Add);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BSub* node) {
  DO_BINARY_EXPR(Sub);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BDiv* node) {
  DO_BINARY_EXPR(Div);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BMul* node) {
  DO_BINARY_EXPR(Mul);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BMod* node) {
  DO_BINARY_EXPR(Mod);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BSquare* node) {
  DO_BINARY_EXPR(Pow);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BAnd* node) {
  DO_BINARY_EXPR(And);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BOr* node) {
  DO_BINARY_EXPR(Or);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BXor* node) {
  DO_BINARY_EXPR(Xor);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BBitAnd* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BBitOr* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BBitXor* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BBitLeftShift* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BBitRightShift* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BConcat* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BIndex* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BLike* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BIn* node) {
  auto l_val = std::any_cast<Entry>(node->left()->accept(*this));
  auto r_val = std::any_cast<Entry>(node->right()->accept(*this));
  if (!l_val.IsScalar()) NOT_SUPPORT_AND_THROW();
  if (!r_val.IsArray()) NOT_SUPPORT_AND_THROW();
  for (auto& val : r_val.constant.AsArray()) {
    if (l_val.constant == val) {
      return Entry(Value(true));
    }
  }
  return Entry(Value(false));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::If* node) {
  NOT_SUPPORT_AND_THROW();
}

bool cypher::AstExprEvaluator::TryLoadCachedNumericVector(
    geax::frontend::Expr* expr, NumericVectorView* view) {
  auto param = dynamic_cast<geax::frontend::Param*>(expr);
  if (param == nullptr) {
    return false;
  }
  const auto& variable = param->name();
  auto param_iter = ctx_->bolt_parameters_.find(variable);
  if (param_iter == ctx_->bolt_parameters_.end()) {
    THROW_CODE(CypherException, "Parameter {} missing value", variable);
  }

  auto iter = numeric_vector_cache_.find(param_iter->second);
  if (iter == numeric_vector_cache_.end()) {
    auto entry = EvalConstantExpr(ctx_, param_iter->second);
    if (!entry.IsConstant() || !entry.constant.IsArray()) {
      THROW_CODE(CypherException,
                 "Invalid argument of vector function: expected List of "
                 "numeric values");
    }
    CachedNumericVector cached;
    const auto& values = entry.constant.AsArray();
    cached.values.reserve(values.size());
    for (const auto& item : values) {
      double value = NumericValueAsDouble(item, "vector function");
      cached.values.emplace_back(value);
      cached.norm_sq += value * value;
    }
    iter = numeric_vector_cache_.emplace(param_iter->second, std::move(cached))
               .first;
  }
  view->storage = NumericVectorStorage::DOUBLE;
  view->doubles = &iter->second.values;
  view->raw_floats = nullptr;
  view->norm_sq = iter->second.norm_sq;
  return true;
}

bool cypher::AstExprEvaluator::TryLoadNodeVectorField(
    geax::frontend::Expr* expr, NumericVectorScratch* scratch,
    NumericVectorView* view) {
  auto field = dynamic_cast<geax::frontend::GetField*>(expr);
  if (field == nullptr) {
    return false;
  }
  const std::string& field_name = field->fieldName();
  auto pid_iter = property_pid_cache_.find(field_name);
  if (pid_iter == property_pid_cache_.end()) {
    auto pid = ctx_->txn_->db()->id_generator().GetPid(field_name);
    if (!pid.has_value()) {
      return false;
    }
    pid_iter = property_pid_cache_.emplace(field_name, pid.value()).first;
  }

  auto ref = dynamic_cast<geax::frontend::Ref*>(field->expr());
  Entry entry;
  if (ref != nullptr) {
    auto symbol = sym_tab_->symbols.find(ref->name());
    if (symbol == sym_tab_->symbols.end() ||
        record_->values.size() <= static_cast<size_t>(symbol->second.id)) {
      return false;
    }
    entry = record_->values[symbol->second.id];
  } else {
    entry = std::any_cast<Entry>(field->expr()->accept(*this));
  }
  if (!entry.IsNode() || !entry.node->vertex_) {
    return false;
  }
  if (!entry.node->vertex_->TryGetVectorPropertyRaw(
          pid_iter->second, &scratch->raw_floats, &scratch->raw_dimensions)) {
    return false;
  }
  view->storage = NumericVectorStorage::RAW_FLOAT;
  view->doubles = nullptr;
  view->raw_floats = &scratch->raw_floats;
  view->raw_dimensions = scratch->raw_dimensions;
  view->norm_sq = -1.0;
  return true;
}

void cypher::AstExprEvaluator::LoadNumericVector(geax::frontend::Expr* expr,
                                                 NumericVectorScratch* scratch,
                                                 NumericVectorView* view) {
  if (TryLoadCachedNumericVector(expr, view)) {
    return;
  }
  if (TryLoadNodeVectorField(expr, scratch, view)) {
    return;
  }

  auto entry = std::any_cast<Entry>(expr->accept(*this));
  if (!entry.IsConstant() || !entry.constant.IsArray()) {
    THROW_CODE(CypherException,
               "Invalid argument of vector function: expected List of numeric "
               "values");
  }
  const auto& values = entry.constant.AsArray();
  scratch->doubles.clear();
  scratch->doubles.reserve(values.size());
  double norm_sq = 0.0;
  for (const auto& item : values) {
    double value = NumericValueAsDouble(item, "vector function");
    scratch->doubles.emplace_back(value);
    norm_sq += value * value;
  }
  view->storage = NumericVectorStorage::DOUBLE;
  view->doubles = &scratch->doubles;
  view->raw_floats = nullptr;
  view->norm_sq = norm_sq;
}

bool cypher::AstExprEvaluator::TryEvaluateVectorSimilarityFunction(
    geax::frontend::Function* node, const std::string& func_name,
    Entry* result) {
  bool is_cosine = func_name == "vector.similarity.cosine";
  bool is_l2 = func_name == "vector.distance.l2";
  bool is_inner_product = func_name == "vector.similarity.inner_product";
  if (!is_cosine && !is_l2 && !is_inner_product) {
    return false;
  }
  if (node->args().size() != 2) CYPHER_ARGUMENT_ERROR();

  NumericVectorScratch lhs_scratch;
  NumericVectorScratch rhs_scratch;
  NumericVectorView lhs;
  NumericVectorView rhs;
  LoadNumericVector(node->args()[0], &lhs_scratch, &lhs);
  LoadNumericVector(node->args()[1], &rhs_scratch, &rhs);
  if (lhs.Size() != rhs.Size()) {
    THROW_CODE(CypherException, "Invalid argument of " + func_name +
                                    ": vectors must have the same dimension");
  }
  if (lhs.Size() == 0) {
    *result = Entry(Value());
    return true;
  }

  auto get = [](const NumericVectorView& vector, size_t i) -> double {
    if (vector.storage == NumericVectorStorage::RAW_FLOAT) {
      return RawFloatAt(*vector.raw_floats, i);
    }
    return (*vector.doubles)[i];
  };

  double lhs_norm_sq = lhs.norm_sq;
  double rhs_norm_sq = rhs.norm_sq;
  bool compute_lhs_norm = is_cosine && lhs_norm_sq < 0.0;
  bool compute_rhs_norm = is_cosine && rhs_norm_sq < 0.0;
  if (compute_lhs_norm) lhs_norm_sq = 0.0;
  if (compute_rhs_norm) rhs_norm_sq = 0.0;

  double dot = 0.0;
  double l2_sum = 0.0;
  for (size_t i = 0; i < lhs.Size(); ++i) {
    double x = get(lhs, i);
    double y = get(rhs, i);
    if (is_l2) {
      double diff = x - y;
      l2_sum += diff * diff;
    } else {
      dot += x * y;
      if (compute_lhs_norm) lhs_norm_sq += x * x;
      if (compute_rhs_norm) rhs_norm_sq += y * y;
    }
  }

  if (is_l2) {
    *result = Entry(Value(std::sqrt(l2_sum)));
  } else if (is_inner_product) {
    *result = Entry(Value(dot));
  } else {
    if (lhs_norm_sq == 0.0 || rhs_norm_sq == 0.0) {
      *result = Entry(Value());
    } else {
      *result =
          Entry(Value(dot / (std::sqrt(lhs_norm_sq) * std::sqrt(rhs_norm_sq))));
    }
  }
  return true;
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Function* node) {
  static std::unordered_map<std::string, BuiltinFunction::FUNC>
      ae_registered_funcs = ArithOpNode::RegisterFuncs();
  std::string func_name = node->name();
  std::transform(func_name.begin(), func_name.end(), func_name.begin(),
                 ::tolower);
  Entry vector_result;
  if (TryEvaluateVectorSimilarityFunction(node, func_name, &vector_result)) {
    return vector_result;
  }
  auto it = ae_registered_funcs.find(func_name);
  if (it != ae_registered_funcs.end()) {
    std::vector<ArithExprNode> args;
    args.emplace_back(node, *sym_tab_);
    for (auto i : node->args()) {
      args.emplace_back(i, *sym_tab_);
    }
    return Entry(it->second(ctx_, *record_, args));
  }
  THROW_CODE(InputError, "Func [{}] does not exist.", func_name);
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Case* node) {
  if (node->input().has_value()) {
    auto l_val = std::any_cast<Entry>(node->input().value()->accept(*this));
    for (auto& [cond, expr] : node->caseBodies()) {
      auto r_val = std::any_cast<Entry>(cond->accept(*this));
      if (l_val == r_val) {
        return expr->accept(*this);
      }
    }
    if (node->elseBody().has_value()) {
      return node->elseBody().value()->accept(*this);
    } else {
      return Entry(Value());
    }

  } else {
    for (auto& [cond, expr] : node->caseBodies()) {
      auto cond_val = std::any_cast<Entry>(cond->accept(*this));
      if (!cond_val.IsBool()) {
        NOT_SUPPORT_AND_THROW();
      }
      if (cond_val.constant.AsBool()) {
        return expr->accept(*this);
      }
    }
    if (node->elseBody().has_value()) {
      return node->elseBody().value()->accept(*this);
    } else {
      NOT_SUPPORT_AND_THROW();
    }
  }
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Cast* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MatchCase* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::AggFunc* node) {
  // CYPHER_TODO();
  std::unordered_map<std::string, std::function<std::shared_ptr<AggCtx>()>>
      registered_agg_funcs = ArithOpNode::RegisterAggFuncs();
  std::string func_name = ToString(node->funcName());
  std::transform(func_name.begin(), func_name.end(), func_name.begin(),
                 ::tolower);
  auto agg_it = registered_agg_funcs.find(func_name);
  if (agg_it != registered_agg_funcs.end()) {
    // Evalute Mode
    if (visit_mode_ == VisitMode::EVALUATE) {
      if (agg_pos_ >= agg_ctxs_.size()) {
        if (func_name == "count") {
          return Entry(Value::Integer(0));
        } else {
          return Entry(Value());
        }
      }
      return agg_ctxs_[agg_pos_++]->result;
    } else if (visit_mode_ == VisitMode::AGGREGATE) {
      // todo(...): registered_agg_funcs cannot be static and need improvement
      // return Entry(agg_it->second());
      if (agg_pos_ == agg_ctxs_.size()) {
        agg_ctxs_.emplace_back(agg_it->second());
      }
      if (agg_pos_ >= agg_ctxs_.size()) {
        NOT_SUPPORT_AND_THROW();
      }
      std::vector<Entry> args;
      if (func_name == "count" &&
          node->expr()->type() == geax::frontend::AstNodeType::kVString &&
          ((geax::frontend::VString*)node->expr())->val() == "*" &&
          record_->Null()) {
        args.emplace_back(Value());
      } else {
        args.emplace_back(Value(node->isDistinct()));
        args.emplace_back(std::any_cast<Entry>(node->expr()->accept(*this)));
      }
      agg_ctxs_[agg_pos_]->Step(args);
      return Entry(Value());
    }
  }
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::BAggFunc* node) {
  std::unordered_map<std::string, std::function<std::shared_ptr<AggCtx>()>>
      registered_agg_funcs = ArithOpNode::RegisterAggFuncs();
  std::string func_name = ToString(node->funcName());
  std::transform(func_name.begin(), func_name.end(), func_name.begin(),
                 ::tolower);
  auto agg_it = registered_agg_funcs.find(func_name);
  if (agg_it != registered_agg_funcs.end()) {
    // Evalute Mode
    if (visit_mode_ == VisitMode::EVALUATE) {
      if (agg_pos_ >= agg_ctxs_.size()) {
        return Entry(Value());
      }
      return agg_ctxs_[agg_pos_++]->result;
    } else if (visit_mode_ == VisitMode::AGGREGATE) {
      // todo(...): registered_agg_funcs cannot be static and need improvement
      // return Entry(agg_it->second());
      if (agg_pos_ == agg_ctxs_.size()) {
        agg_ctxs_.emplace_back(agg_it->second());
      }
      if (agg_pos_ >= agg_ctxs_.size()) {
        NOT_SUPPORT_AND_THROW();
      }
      std::vector<Entry> args;
      auto& left = node->lExpr();
      args.emplace_back(Entry(Value(std::get<0>(left))));
      args.emplace_back(std::any_cast<Entry>(std::get<1>(left)->accept(*this)));
      args.emplace_back(std::any_cast<Entry>(node->rExpr()->accept(*this)));
      agg_ctxs_[agg_pos_]->Step(args);
      return Entry(Value());
    }
  }
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MultiCount* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Windowing* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MkList* node) {
  const auto& elems = node->elems();
  std::vector<Value> list;
  for (auto& e : elems) {
    auto entry = std::any_cast<Entry>(e->accept(*this));
    if (!entry.IsConstant()) NOT_SUPPORT_AND_THROW();
    list.emplace_back(std::move(entry.constant));
  }
  return Entry(Value(std::move(list)));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MkMap* node) {
  const auto& elems = node->elems();
  std::unordered_map<std::string, Value> map;
  for (const auto& pair : elems) {
    auto key = std::any_cast<Entry>(std::get<0>(pair)->accept(*this));
    auto val = std::any_cast<Entry>(std::get<1>(pair)->accept(*this));
    if (!key.IsString()) NOT_SUPPORT_AND_THROW();
    if (!val.IsConstant()) NOT_SUPPORT_AND_THROW();
    map.emplace(key.constant.AsString(), std::move(val.constant));
  }
  return Entry(Value(std::move(map)));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MkRecord* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MkSet* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::MkTuple* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VBool* node) {
  return Entry(Value(node->val()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VInt* node) {
  return Entry(Value(node->val()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VDouble* node) {
  return Entry(Value(node->val()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VString* node) {
  return Entry(Value(node->val()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VDate* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VDatetime* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VDuration* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VTime* node) {
  NOT_SUPPORT_AND_THROW();
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VNull* node) {
  return Entry(Value());
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::VNone* node) {
  NOT_SUPPORT_AND_THROW();
}

/* Creates a path from a given sequence of graph entities.
 * The arguments are the sequence of graph entities combines the path.
 * The sequence is always in odd length and defined as:
 * Odd indices members are always representing the value of a single node.
 * Even indices members are either representing the value of a single edge,
 * or an sipath, in case of variable length traversal.  */
Entry ToPath(RTContext* ctx, const Record& record,
             const std::vector<ArithExprNode>& args) {
  if (args.size() % 2 == 0) CYPHER_ARGUMENT_ERROR();
  Entry ret;
  ret.type = Entry::PATH;
  for (auto& arg : args) {
    auto r = arg.Evaluate(ctx, record);
    if (r.type == Entry::VAR_LEN_RELP) {
      // CYPHER_TODO();
      ret.path.clear();
      int len = int(r.relationship->path_.Length());
      for (int i = 0; i < len; ++i) {
        if (i != 0) {
          ret.path.push_back(
              PathElement{false, r.relationship->path_.edges[i - 1]});
        }
        ret.path.push_back(
            PathElement{true, r.relationship->path_.vertexes[i]});
      }
      break;
    } else {
      if ((r.IsNode() && !r.node->vertex_) ||
          (r.IsRelationship() && !r.relationship->edge_)) {
        ret.path.clear();
        break;
      }
      if (r.IsNode()) {
        ret.path.emplace_back(PathElement{true, r.node->vertex_.value()});
      } else if (r.IsRelationship()) {
        ret.path.emplace_back(
            PathElement{false, r.relationship->edge_.value()});
      } else {
        // CYPHER_TODO();
      }
    }
  }
  return ret;
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Ref* node) {
  auto it = sym_tab_->symbols.find(node->name());
  if (it == sym_tab_->symbols.end()) NOT_SUPPORT_AND_THROW();
  switch (it->second.type) {
    case SymbolNode::NODE:
    case SymbolNode::RELATIONSHIP:
    case SymbolNode::CONSTANT:
    case SymbolNode::PARAMETER:
      return record_->values[it->second.id];
    case SymbolNode::NAMED_PATH: {
      auto iter = sym_tab_->anot_collection.path_elements.find(node->name());
      if (iter == sym_tab_->anot_collection.path_elements.end())
        THROW_CODE(CypherException, "path_elements error: " + node->name());
      const std::vector<std::shared_ptr<geax::frontend::Ref>>& elements =
          iter->second;
      std::vector<ArithExprNode> params;
      params.reserve(elements.size());
      for (const auto& ref : elements) {
        params.emplace_back(ref.get(), *sym_tab_);
      }
      return ToPath(ctx_, *record_, params);
      // return Entry(doCallBuiltinFunc(BuiltinFunction::INTL_TO_PATH, ctx_,
      // *record_, params));
    }
    default: {
      CYPHER_TODO();
    }
  }
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Param* node) {
  const auto& variable = node->name();
  auto it = ctx_->bolt_parameters_.find(variable);
  if (it == ctx_->bolt_parameters_.end()) {
    THROW_CODE(CypherException, "Parameter {} missing value", variable);
  }
  auto entry = EvalConstantExpr(ctx_, it->second);
  if (!entry.IsConstant()) {
    THROW_CODE(CypherException, "Parameter {} is not a scalar value", variable);
  }
  return entry;
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::SingleLabel* node) {
  std::unordered_set<std::string> set;
  set.insert(node->label());
  return set;
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::LabelOr* node) {
  std::unordered_set<std::string> left;
  left = std::any_cast<std::unordered_set<std::string>>(
      node->left()->accept(*this));
  std::unordered_set<std::string> right;
  right = std::any_cast<std::unordered_set<std::string>>(
      node->right()->accept(*this));
  left.insert(right.begin(), right.end());
  return left;
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::IsLabeled* node) {
  auto e = std::any_cast<Entry>(node->expr()->accept(*this));
  CYPHER_THROW_ASSERT(e.IsNode() || e.IsRelationship());
  auto expect = std::any_cast<std::unordered_set<std::string>>(
      node->labelTree()->accept(*this));
  std::unordered_set<std::string> labels;
  if (e.IsNode()) {
    labels = e.node->vertex_->GetLabels();
  } else if (e.IsRelationship()) {
    auto type = e.relationship->edge_->GetType();
    labels.insert(type);
  }
  std::unordered_set<std::string> res;
  std::set_intersection(expect.begin(), expect.end(), labels.begin(),
                        labels.end(), std::inserter(res, res.begin()));

  return Entry(Value(!res.empty()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::IsNull* node) {
  Value ret;
  auto ans = std::any_cast<Entry>(node->expr()->accept(*this));
  return Entry(Value::Bool(ans.constant.IsNull()));
}

std::any cypher::AstExprEvaluator::visit(geax::frontend::Exists* node) {
  auto path_chains = node->pathChains();
  if (path_chains.size() > 1) NOT_SUPPORT_AND_THROW();
  auto head = path_chains[0]->head();
  const std::string head_name = getNodeOrEdgeName(head);
  auto it_head = sym_tab_->symbols.find(head_name);
  if (it_head == sym_tab_->symbols.end() ||
      it_head->second.type != SymbolNode::NODE)
    NOT_SUPPORT_AND_THROW();
  CYPHER_TODO();
  /*if (!record_->values[it_head->second.id].CheckEntityEfficient(ctx_))
      return Entry(cypher::FieldData(PropertyValue(false)));

  auto& tails = path_chains[0]->tails();
  for (auto& tail : tails) {
      auto relationship = std::get<0>(tail);
      const std::string rel_name = getNodeOrEdgeName(relationship);
      auto it_rel = sym_tab_->symbols.find(rel_name);
      if (it_rel == sym_tab_->symbols.end() || it_rel->second.type !=
  SymbolNode::RELATIONSHIP) NOT_SUPPORT_AND_THROW(); if
  (!record_->values[it_rel->second.id].CheckEntityEfficient(ctx_)) return
  Entry(cypher::FieldData(PropertyValue(false))); auto neighbor =
  std::get<1>(tail); const std::string ne_name = getNodeOrEdgeName(neighbor);
      auto it_ne = sym_tab_->symbols.find(ne_name);
      if (it_ne == sym_tab_->symbols.end() || it_ne->second.type !=
  SymbolNode::NODE) NOT_SUPPORT_AND_THROW(); if
  (!record_->values[it_ne->second.id].CheckEntityEfficient(ctx_)) return
  Entry(cypher::FieldData(PropertyValue(false)));
  }
  return Entry(cypher::FieldData(PropertyValue(true)));*/
}

std::any cypher::AstExprEvaluator::reportError() { return error_msg_; }

std::any AstExprEvaluator::visit(geax::frontend::ListComprehension* node) {
  geax::frontend::Ref* ref = nullptr;
  geax::frontend::Expr *in_expr = nullptr, *op_expr = nullptr,
                       *where_expr = nullptr;
  checkedCast(node->getVariable(), ref);
  checkedCast(node->getInExpression(), in_expr);
  if (node->getWhereExpression() != nullptr) {
    checkedCast(node->getWhereExpression(), where_expr);
  }
  checkedCast(node->getOpExpression(), op_expr);
  Entry in_e;
  in_e = std::any_cast<Entry>(in_expr->accept(*this));
  CYPHER_THROW_ASSERT(in_e.IsArray());
  const auto& data_array = in_e.constant.AsArray();
  std::vector<Value> ret_data;
  auto it = sym_tab_->symbols.find(ref->name());
  for (auto& data : data_array) {
    const_cast<Record*>(record_)->values[it->second.id] = Entry(data);
    Entry one_result;
    one_result = std::any_cast<Entry>(op_expr->accept(*this));
    if (where_expr != nullptr) {
      auto where = std::any_cast<Entry>(where_expr->accept(*this));
      if (!where.constant.AsBool()) {
        continue;
      }
    }
    ret_data.push_back(one_result.constant);
  }
  return Entry(Value(ret_data));
}

std::any AstExprEvaluator::visit(geax::frontend::PredicateFunction* node) {
  geax::frontend::Ref* ref = nullptr;
  geax::frontend::Expr *in_expr = nullptr, *where_expr = nullptr;
  checkedCast(node->getVariable(), ref);
  checkedCast(node->getInExpression(), in_expr);
  checkedCast(node->getWhereExpression(), where_expr);
  auto predicateType = cypher::PredicateType(node->getPredicateType());
  Entry in_e;
  in_e = std::any_cast<Entry>(in_expr->accept(*this));
  CYPHER_THROW_ASSERT(in_e.IsArray() || in_e.IsNull());
  if (in_e.IsNull()) {
    return Entry(Value());
  }
  const auto& data_array = in_e.constant.AsArray();
  std::vector<Value> ret_data;
  auto it = sym_tab_->symbols.find(ref->name());
  for (auto& data : data_array) {
    const_cast<Record*>(record_)->values[it->second.id] = Entry(data);
    Entry one_result;
    one_result = std::any_cast<Entry>(where_expr->accept(*this));
    ret_data.push_back(one_result.constant);
  }
  switch (predicateType) {
    case cypher::PredicateType::None: {
      bool ans = true, isnull = false;
      for (const auto& r : ret_data) {
        if (r.IsNull()) {
          isnull = true;
        } else {
          ans &= !r.AsBool();
        }
      }
      if (isnull && ans) {
        return Entry(Value());
      }
      return Entry(Value(ans));
    }
    case cypher::PredicateType::Single: {
      int count = 0;
      bool isnull = false;
      for (const auto& r : ret_data) {
        if (r.IsNull()) {
          isnull = true;
        } else if (r.IsBool()) {
          count++;
        }
      }
      if (isnull && count == 0) {
        return Entry(Value());
      } else {
        return Entry(Value(count == 1));
      }
      break;
    }
    case cypher::PredicateType::Any: {
      bool ans = false, isnull = false;
      for (const auto& r : ret_data) {
        if (r.IsNull()) {
          isnull = true;
        } else {
          ans = true;
        }
      }
      if (isnull && !ans) {
        return Entry(Value());
      }
      return Entry(Value(ans));
    }
    case cypher::PredicateType::All: {
      bool ans = true, isnull = false;
      for (const auto& r : ret_data) {
        if (r.IsNull()) {
          isnull = true;
        } else {
          ans &= r.AsBool();
        }
      }
      if (isnull && ans) {
        return Entry(Value());
      }
      return Entry(Value(ans));
    }
  }
  return Entry();
}

}  // namespace cypher
