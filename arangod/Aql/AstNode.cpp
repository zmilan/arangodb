////////////////////////////////////////////////////////////////////////////////
/// @brief Aql, AST node
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2014 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Copyright 2014, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2012-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "Aql/AstNode.h"
#include "Aql/Ast.h"
#include "Aql/Executor.h"
#include "Aql/Function.h"
#include "Aql/Scopes.h"
#include "Aql/types.h"
#include "Basics/JsonHelper.h"
#include "Basics/json-utilities.h"
#include "Basics/StringBuffer.h"
#include "Basics/utf8-helper.h"

using namespace triagens::aql;
using JsonHelper = triagens::basics::JsonHelper;
using Json = triagens::basics::Json;

// -----------------------------------------------------------------------------
// --SECTION--                                            static initializations
// -----------------------------------------------------------------------------

std::unordered_map<int, std::string const> const AstNode::Operators{ 
  { static_cast<int>(NODE_TYPE_OPERATOR_UNARY_NOT),       "!" },
  { static_cast<int>(NODE_TYPE_OPERATOR_UNARY_PLUS),      "+" },
  { static_cast<int>(NODE_TYPE_OPERATOR_UNARY_MINUS),     "-" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_AND),      "&&" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_OR),       "||" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_PLUS),     "+" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_MINUS),    "-" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_TIMES),    "*" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_DIV),      "/" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_MOD),      "%" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_EQ),       "==" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_NE),       "!=" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_LT),       "<" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_LE),       "<=" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_GT),       ">" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_GE),       ">=" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_IN),       "IN" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_NIN),      "NOT IN" }
};

std::unordered_map<int, std::string const> const AstNode::TypeNames{ 
  { static_cast<int>(NODE_TYPE_ROOT),                     "root" },
  { static_cast<int>(NODE_TYPE_FOR),                      "for" },
  { static_cast<int>(NODE_TYPE_LET),                      "let" },
  { static_cast<int>(NODE_TYPE_FILTER),                   "filter" },
  { static_cast<int>(NODE_TYPE_RETURN),                   "return" },
  { static_cast<int>(NODE_TYPE_REMOVE),                   "remove" },
  { static_cast<int>(NODE_TYPE_INSERT),                   "insert" },
  { static_cast<int>(NODE_TYPE_UPDATE),                   "update" },
  { static_cast<int>(NODE_TYPE_REPLACE),                  "replace" },
  { static_cast<int>(NODE_TYPE_COLLECT),                  "collect" },
  { static_cast<int>(NODE_TYPE_SORT),                     "sort" },
  { static_cast<int>(NODE_TYPE_SORT_ELEMENT),             "sort element" },
  { static_cast<int>(NODE_TYPE_LIMIT),                    "limit" },
  { static_cast<int>(NODE_TYPE_VARIABLE),                 "variable" },
  { static_cast<int>(NODE_TYPE_ASSIGN),                   "assign" },
  { static_cast<int>(NODE_TYPE_OPERATOR_UNARY_PLUS),      "unary plus" },
  { static_cast<int>(NODE_TYPE_OPERATOR_UNARY_MINUS),     "unary minus" },
  { static_cast<int>(NODE_TYPE_OPERATOR_UNARY_NOT),       "unary not" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_AND),      "logical and" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_OR),       "logical or" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_PLUS),     "plus" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_MINUS),    "minus" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_TIMES),    "times" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_DIV),      "division" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_MOD),      "modulus" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_EQ),       "compare ==" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_NE),       "compare !=" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_LT),       "compare <" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_LE),       "compare <=" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_GT),       "compare >" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_GE),       "compare >=" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_IN),       "compare in" },
  { static_cast<int>(NODE_TYPE_OPERATOR_BINARY_NIN),      "compare not in" },
  { static_cast<int>(NODE_TYPE_OPERATOR_TERNARY),         "ternary" },
  { static_cast<int>(NODE_TYPE_SUBQUERY),                 "subquery" },
  { static_cast<int>(NODE_TYPE_ATTRIBUTE_ACCESS),         "attribute access" },
  { static_cast<int>(NODE_TYPE_BOUND_ATTRIBUTE_ACCESS),   "bound attribute access" },
  { static_cast<int>(NODE_TYPE_INDEXED_ACCESS),           "indexed access" },
  { static_cast<int>(NODE_TYPE_EXPAND),                   "expand" },
  { static_cast<int>(NODE_TYPE_ITERATOR),                 "iterator" },
  { static_cast<int>(NODE_TYPE_VALUE),                    "value" },
  { static_cast<int>(NODE_TYPE_LIST),                     "list" },
  { static_cast<int>(NODE_TYPE_ARRAY),                    "array" },
  { static_cast<int>(NODE_TYPE_ARRAY_ELEMENT),            "array element" },
  { static_cast<int>(NODE_TYPE_COLLECTION),               "collection" },
  { static_cast<int>(NODE_TYPE_REFERENCE),                "reference" },
  { static_cast<int>(NODE_TYPE_PARAMETER),                "parameter" },
  { static_cast<int>(NODE_TYPE_FCALL),                    "function call" },
  { static_cast<int>(NODE_TYPE_FCALL_USER),               "user function call" },
  { static_cast<int>(NODE_TYPE_RANGE),                    "range" },
  { static_cast<int>(NODE_TYPE_NOP),                      "no-op" }
};

std::unordered_map<int, std::string const> const AstNode::valueTypeNames{
  { static_cast<int>(VALUE_TYPE_NULL),                    "null" },
  { static_cast<int>(VALUE_TYPE_BOOL),                    "bool" },
  { static_cast<int>(VALUE_TYPE_INT),                     "int" },
  { static_cast<int>(VALUE_TYPE_DOUBLE),                  "double" },
  { static_cast<int>(VALUE_TYPE_STRING),                  "string" }
};

// -----------------------------------------------------------------------------
// --SECTION--                                           static helper functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief get the node type for inter-node comparisons
////////////////////////////////////////////////////////////////////////////////

static TRI_json_type_e GetNodeCompareType (AstNode const* node) {
  if (node->type == NODE_TYPE_VALUE) {
    switch (node->value.type) {
      case VALUE_TYPE_NULL:
        return TRI_JSON_NULL;
      case VALUE_TYPE_BOOL:
        return TRI_JSON_BOOLEAN;
      case VALUE_TYPE_INT:
      case VALUE_TYPE_DOUBLE:
        // numbers are treated the same here
        return TRI_JSON_NUMBER;
      case VALUE_TYPE_STRING:
        return TRI_JSON_STRING;
    }
  }
  else if (node->type == NODE_TYPE_LIST) {
    return TRI_JSON_LIST;
  }
  else if (node->type == NODE_TYPE_ARRAY) {
    return TRI_JSON_ARRAY;
  }
  TRI_ASSERT(false);
  return TRI_JSON_NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief compare two nodes
////////////////////////////////////////////////////////////////////////////////

static int CompareNodes (AstNode const* lhs, AstNode const* rhs) {
  auto lType = GetNodeCompareType(lhs);
  auto rType = GetNodeCompareType(rhs);

  if (lType != rType) {
    return static_cast<int>(lType) - static_cast<int>(rType);
  }

  if (lType == TRI_JSON_BOOLEAN) {
    return static_cast<int>(lhs->getIntValue() - rhs->getIntValue());
  }
  else if (lType == TRI_JSON_NUMBER) {
    double d = lhs->getDoubleValue() - rhs->getDoubleValue();
    if (d < 0.0) {
      return -1;
    } 
    else if (d > 0.0) {
      return 1;
    }
  }
  else if (lType == TRI_JSON_STRING) {
    return TRI_compare_utf8(lhs->getStringValue(), rhs->getStringValue());
  }
  else if (lType == TRI_JSON_LIST) {
    size_t const numLhs = lhs->numMembers();
    size_t const numRhs = rhs->numMembers();
    size_t const n = ((numLhs > numRhs) ? numRhs : numLhs);
 
    for (size_t i = 0; i < n; ++i) {
      int res = CompareNodes(lhs->getMember(i), rhs->getMember(i));
      if (res != 0) {
        return res;
      }    
    }
    if (numLhs < numRhs) {
      return -1;
    }
    else if (numLhs > numRhs) {
      return 1;
    }
  }
  else if (lType == TRI_JSON_ARRAY) {
    // this is a rather exceptional case, so we can
    // afford the inefficiency to convert to node to
    // JSON for comparison
    // (this saves us from writing our own compare function
    // for array AstNodes)
    auto lJson = lhs->toJsonValue(TRI_UNKNOWN_MEM_ZONE);
    auto rJson = lhs->toJsonValue(TRI_UNKNOWN_MEM_ZONE);
    int res = TRI_CompareValuesJson(lJson, rJson, true);
    if (lJson != nullptr) {
      TRI_FreeJson(TRI_UNKNOWN_MEM_ZONE, lJson);
    }
    if (rJson != nullptr) {
      TRI_FreeJson(TRI_UNKNOWN_MEM_ZONE, rJson);
    }
    return res;
  }

  // all things equal
  return 0;
}

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief create the node
////////////////////////////////////////////////////////////////////////////////

AstNode::AstNode (AstNodeType type)
  : type(type),
    flags(0),
    computedJson(nullptr) {

  TRI_InitVectorPointer(&members, TRI_UNKNOWN_MEM_ZONE);
  TRI_ASSERT(flags == 0);
  TRI_ASSERT(computedJson == nullptr);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create a node, with defining a value type
////////////////////////////////////////////////////////////////////////////////

AstNode::AstNode (AstNodeType type,
                  AstNodeValueType valueType)
  : AstNode(type) {

  value.type = valueType;
  TRI_ASSERT(flags == 0);
  TRI_ASSERT(computedJson == nullptr);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create a boolean node, with defining a value
////////////////////////////////////////////////////////////////////////////////

AstNode::AstNode (bool v)
  : AstNode(NODE_TYPE_VALUE, VALUE_TYPE_BOOL) {

  value.value._bool = v;
  TRI_ASSERT(flags == 0);
  TRI_ASSERT(computedJson == nullptr);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create the node from JSON
////////////////////////////////////////////////////////////////////////////////

AstNode::AstNode (Ast* ast,
                  triagens::basics::Json const& json) 
  : AstNode(getNodeTypeFromJson(json)) {

  TRI_ASSERT(flags == 0);
  TRI_ASSERT(computedJson == nullptr);

  auto query = ast->query();

  switch (type) {
    case NODE_TYPE_COLLECTION:
    case NODE_TYPE_PARAMETER:
    case NODE_TYPE_ATTRIBUTE_ACCESS:
    case NODE_TYPE_FCALL_USER:
      value.type = VALUE_TYPE_STRING;
      setStringValue(query->registerString(JsonHelper::getStringValue(json.json(), "name", ""), false));
      break;
    case NODE_TYPE_VALUE: {
      int vType = JsonHelper::checkAndGetNumericValue<int>(json.json(), "vTypeID");
      validateValueType(vType);
      value.type = static_cast<AstNodeValueType>(vType);

      switch (value.type) {
        case VALUE_TYPE_NULL:
          break;
        case VALUE_TYPE_BOOL:
          value.value._bool = JsonHelper::checkAndGetBooleanValue(json.json(), "value");
          break;
        case VALUE_TYPE_INT:
          setIntValue(JsonHelper::checkAndGetNumericValue<int64_t>(json.json(), "value"));
          break;
        case VALUE_TYPE_DOUBLE:
          setDoubleValue(JsonHelper::checkAndGetNumericValue<double>(json.json(), "value"));
          break;
        case VALUE_TYPE_STRING:
          setStringValue(query->registerString(JsonHelper::checkAndGetStringValue(json.json(), "value"), false));
          break;
        default: {
        }
      }
      break;
    }
    case NODE_TYPE_VARIABLE: {
      auto variable = ast->variables()->createVariable(json);
      TRI_ASSERT(variable != nullptr);
      setData(variable);
      break;
    }
    case NODE_TYPE_REFERENCE: {
      auto variableId = JsonHelper::checkAndGetNumericValue<VariableId>(json.json(), "id");
      auto variable = ast->variables()->getVariable(variableId);

      TRI_ASSERT(variable != nullptr);
      setData(variable);
      break;
    }
    case NODE_TYPE_FCALL: {
      setData(query->executor()->getFunctionByName(JsonHelper::getStringValue(json.json(), "name", "")));
      break;
    }
    case NODE_TYPE_ARRAY_ELEMENT: {
      setStringValue(query->registerString(JsonHelper::getStringValue(json.json(), "name", ""), false));
      break;
    }
    case NODE_TYPE_ARRAY:
    case NODE_TYPE_ROOT:
    case NODE_TYPE_FOR:
    case NODE_TYPE_LET:
    case NODE_TYPE_FILTER:
    case NODE_TYPE_RETURN:
    case NODE_TYPE_REMOVE:
    case NODE_TYPE_INSERT:
    case NODE_TYPE_UPDATE:
    case NODE_TYPE_REPLACE:
    case NODE_TYPE_COLLECT:
    case NODE_TYPE_SORT:
    case NODE_TYPE_SORT_ELEMENT:
    case NODE_TYPE_LIMIT:
    case NODE_TYPE_ASSIGN:
    case NODE_TYPE_OPERATOR_UNARY_PLUS:
    case NODE_TYPE_OPERATOR_UNARY_MINUS:
    case NODE_TYPE_OPERATOR_UNARY_NOT:
    case NODE_TYPE_OPERATOR_BINARY_AND:
    case NODE_TYPE_OPERATOR_BINARY_OR:
    case NODE_TYPE_OPERATOR_BINARY_PLUS:
    case NODE_TYPE_OPERATOR_BINARY_MINUS:
    case NODE_TYPE_OPERATOR_BINARY_TIMES:
    case NODE_TYPE_OPERATOR_BINARY_DIV:
    case NODE_TYPE_OPERATOR_BINARY_MOD:
    case NODE_TYPE_OPERATOR_BINARY_EQ:
    case NODE_TYPE_OPERATOR_BINARY_NE:
    case NODE_TYPE_OPERATOR_BINARY_LT:
    case NODE_TYPE_OPERATOR_BINARY_LE:
    case NODE_TYPE_OPERATOR_BINARY_GT:
    case NODE_TYPE_OPERATOR_BINARY_GE:
    case NODE_TYPE_OPERATOR_BINARY_IN:
    case NODE_TYPE_OPERATOR_BINARY_NIN:
    case NODE_TYPE_OPERATOR_TERNARY:
    case NODE_TYPE_SUBQUERY:
    case NODE_TYPE_BOUND_ATTRIBUTE_ACCESS:
    case NODE_TYPE_INDEXED_ACCESS:
    case NODE_TYPE_EXPAND:
    case NODE_TYPE_ITERATOR:
    case NODE_TYPE_LIST:
    case NODE_TYPE_RANGE:
    case NODE_TYPE_NOP:
      break;
    }

  Json subNodes = json.get("subNodes");
  if (subNodes.isList()) {
    size_t const len = subNodes.size();
    for (size_t i = 0; i < len; i++) {
      Json subNode(subNodes.at(static_cast<int>(i)));
      addMember(new AstNode(ast, subNode));
    }
  }

  ast->query()->addNode(this);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief destroy the node
////////////////////////////////////////////////////////////////////////////////

AstNode::~AstNode () {
  TRI_DestroyVectorPointer(&members);
  
  if (computedJson != nullptr) {
    TRI_FreeJson(TRI_UNKNOWN_MEM_ZONE, computedJson);
    computedJson = nullptr;
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief compute the JSON for a constant value node
/// the JSON is owned by the node and must not be freed by the caller
/// note that the return value might be NULL in case of OOM
////////////////////////////////////////////////////////////////////////////////

TRI_json_t* AstNode::computeJson () const {
  TRI_ASSERT(isConstant());

  if (computedJson == nullptr) {
    // note: the following may fail but we do not need to 
    // check that here
    computedJson = toJsonValue(TRI_UNKNOWN_MEM_ZONE);
  }

  return computedJson;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief sort the members of a (list) node
/// this will also set the FLAG_SORTED flag for the node
////////////////////////////////////////////////////////////////////////////////
  
void AstNode::sort () {
  TRI_ASSERT(type == NODE_TYPE_LIST);
  TRI_ASSERT_EXPENSIVE(isConstant());

  auto const ptr = members._buffer;
  auto const end = ptr + members._length;
  std::sort(ptr, end, [] (void const* lhs, void const* rhs) {
    auto const l = static_cast<AstNode const*>(lhs);
    auto const r = static_cast<AstNode const*>(rhs);

    return (CompareNodes(l, r) < 0);
  });

  setFlag(FLAG_SORTED);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief return the type name of a node
////////////////////////////////////////////////////////////////////////////////

std::string const& AstNode::getTypeString () const {
  auto it = TypeNames.find(static_cast<int>(type));
  if (it != TypeNames.end()) {
    return (*it).second;
  }

  THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "missing node type in TypeNames");
}

////////////////////////////////////////////////////////////////////////////////
/// @brief return the value type name of a node
////////////////////////////////////////////////////////////////////////////////

std::string const& AstNode::getValueTypeString () const {
  auto it = valueTypeNames.find(static_cast<int>(value.type));
  if (it != valueTypeNames.end()) {
    return (*it).second;
  }
  THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "missing node type in valueTypeNames");
}

////////////////////////////////////////////////////////////////////////////////
/// @brief checks whether we know a type of this kind; throws exception if not.
////////////////////////////////////////////////////////////////////////////////

void AstNode::validateType (int type) {
  auto it = TypeNames.find(static_cast<int>(type));
  if (it == TypeNames.end()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "unknown AST-Node TypeID");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief checks whether we know a value type of this kind; 
/// throws exception if not.
////////////////////////////////////////////////////////////////////////////////

void AstNode::validateValueType (int type) {
  auto it = valueTypeNames.find(static_cast<int>(type));
  if (it == valueTypeNames.end()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "invalid AST-Node valueTypeName");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief fetch a node's type from json
////////////////////////////////////////////////////////////////////////////////

AstNodeType AstNode::getNodeTypeFromJson (triagens::basics::Json const& json) {
  int type = JsonHelper::checkAndGetNumericValue<int>(json.json(), "typeID");
  validateType (type);
  return static_cast<AstNodeType>(type);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief return a JSON representation of the node value
/// the caller is responsible for freeing the JSON later
////////////////////////////////////////////////////////////////////////////////

TRI_json_t* AstNode::toJsonValue (TRI_memory_zone_t* zone) const {
  if (type == NODE_TYPE_VALUE) {
    // dump value of "value" node
    switch (value.type) {
      case VALUE_TYPE_NULL:
        return TRI_CreateNullJson(zone);
      case VALUE_TYPE_BOOL:
        return TRI_CreateBooleanJson(zone, value.value._bool);
      case VALUE_TYPE_INT:
        return TRI_CreateNumberJson(zone, static_cast<double>(value.value._int));
      case VALUE_TYPE_DOUBLE:
        return TRI_CreateNumberJson(zone, value.value._double);
      case VALUE_TYPE_STRING:
        return TRI_CreateStringCopyJson(zone, value.value._string);
      default: {
      }
    }
  }
  
  if (type == NODE_TYPE_LIST) {
    size_t const n = numMembers();
    TRI_json_t* list = TRI_CreateList2Json(zone, n);

    if (list == nullptr) {
      return nullptr;
    }

    for (size_t i = 0; i < n; ++i) {
      auto member = getMember(i);

      if (member != nullptr) {
        TRI_json_t* j = member->toJsonValue(zone);

        if (j != nullptr) {
          TRI_PushBack3ListJson(zone, list, j);
        }
      }
    }
    return list;
  }
  
  if (type == NODE_TYPE_ARRAY) {
    size_t const n = numMembers();
    TRI_json_t* array = TRI_CreateArray2Json(zone, n);

    for (size_t i = 0; i < n; ++i) {
      auto member = getMember(i);

      if (member != nullptr) {
        TRI_json_t* j = member->getMember(0)->toJsonValue(zone);

        if (j != nullptr) {
          TRI_Insert3ArrayJson(zone, array, member->getStringValue(), j);
        }
      }
    }

    return array;
  }

  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief return a JSON representation of the node
/// the caller is responsible for freeing the JSON later
////////////////////////////////////////////////////////////////////////////////

TRI_json_t* AstNode::toJson (TRI_memory_zone_t* zone,
                             bool verbose) const {
  TRI_json_t* node = TRI_CreateArrayJson(zone);

  if (node == nullptr) {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
  }

  // dump node type
  auto&& typeString = getTypeString();
  TRI_json_t json;
  if (TRI_InitString2CopyJson(zone, &json, typeString.c_str(), typeString.size()) != TRI_ERROR_NO_ERROR) {
    TRI_FreeJson(zone, node);
    THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
  }

  TRI_Insert2ArrayJson(zone, node, "type", &json); 

  // add typeId
  if (verbose) {
    TRI_Insert3ArrayJson(zone, node, "typeID", TRI_CreateNumberJson(zone, static_cast<int>(type)));
  }

  if (type == NODE_TYPE_COLLECTION ||
      type == NODE_TYPE_PARAMETER ||
      type == NODE_TYPE_ATTRIBUTE_ACCESS ||
      type == NODE_TYPE_ARRAY_ELEMENT ||
      type == NODE_TYPE_FCALL_USER) {
    // dump "name" of node
    if (TRI_InitStringCopyJson(zone, &json, getStringValue()) != TRI_ERROR_NO_ERROR) {
      TRI_FreeJson(zone, node);
      THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
    }
    TRI_Insert2ArrayJson(zone, node, "name", &json);
  }

  if (type == NODE_TYPE_FCALL) {
    auto func = static_cast<Function*>(getData());
    TRI_Insert3ArrayJson(zone, node, "name", TRI_CreateStringCopyJson(zone, func->externalName.c_str()));
    // arguments are exported via node members
  }

  if (type == NODE_TYPE_VALUE) {
    // dump value of "value" node
    auto v = toJsonValue(zone);

    if (v == nullptr) {
      TRI_FreeJson(zone, node);
      THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
    }
    
    TRI_Insert3ArrayJson(zone, node, "value", v);

    if (verbose) {
      TRI_Insert3ArrayJson(zone, node, "vType", TRI_CreateStringCopyJson(zone, getValueTypeString().c_str()));
      TRI_Insert3ArrayJson(zone, node, "vTypeID", TRI_CreateNumberJson(zone, static_cast<int>(value.type)));
    }
  }

  if (type == NODE_TYPE_VARIABLE ||
      type == NODE_TYPE_REFERENCE) {
    auto variable = static_cast<Variable*>(getData());

    TRI_ASSERT(variable != nullptr);

    TRI_Insert3ArrayJson(zone, node, "name", TRI_CreateStringCopyJson(zone, variable->name.c_str()));
    TRI_Insert3ArrayJson(zone, node, "id", TRI_CreateNumberJson(zone, static_cast<double>(variable->id)));
  }
  
  // dump sub-nodes 
  size_t const n = members._length;

  if (n > 0) {
    TRI_json_t* subNodes = TRI_CreateList2Json(zone, n);

    if (subNodes == nullptr) {
      TRI_FreeJson(zone, node);
      THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
    }

    try {
      for (size_t i = 0; i < n; ++i) {
        auto member = getMember(i);
        if (member != nullptr && member->type != NODE_TYPE_NOP) {
          member->toJson(subNodes, zone, verbose);
        }
      }
    }
    catch (...) {
      TRI_FreeJson(zone, subNodes);
      TRI_FreeJson(zone, node);
      THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
    }
    
    TRI_Insert3ArrayJson(zone, node, "subNodes", subNodes);
  }

  return node;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert the node's value to a boolean value
/// this may create a new node or return the node itself if it is already a
/// boolean value node
////////////////////////////////////////////////////////////////////////////////

AstNode* AstNode::castToBool (Ast* ast) {
  TRI_ASSERT(type == NODE_TYPE_VALUE || 
             type == NODE_TYPE_LIST || 
             type == NODE_TYPE_ARRAY);

  if (type == NODE_TYPE_VALUE) {
    switch (value.type) {
      case VALUE_TYPE_NULL:
        // null => false
        return ast->createNodeValueBool(false);
      case VALUE_TYPE_BOOL:
        // already a boolean
        return this;
      case VALUE_TYPE_INT:
        return ast->createNodeValueBool(value.value._int != 0);
      case VALUE_TYPE_DOUBLE:
        return ast->createNodeValueBool(value.value._double != 0.0);
      case VALUE_TYPE_STRING:
        return ast->createNodeValueBool(*(value.value._string) != '\0');
      default: {
      }
    }
    // fall-through intentional
  }
  else if (type == NODE_TYPE_LIST) {
    return ast->createNodeValueBool(true);
  }
  else if (type == NODE_TYPE_ARRAY) {
    return ast->createNodeValueBool(true);
  }

  return ast->createNodeValueBool(false);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert the node's value to a number value
/// this may create a new node or return the node itself if it is already a
/// numeric value node
////////////////////////////////////////////////////////////////////////////////

AstNode* AstNode::castToNumber (Ast* ast) {
  TRI_ASSERT(type == NODE_TYPE_VALUE || 
             type == NODE_TYPE_LIST || 
             type == NODE_TYPE_ARRAY);

  if (type == NODE_TYPE_VALUE) {
    switch (value.type) {
      case VALUE_TYPE_NULL:
        // null => 0
        return ast->createNodeValueInt(0);
      case VALUE_TYPE_BOOL:
        // false => 0, true => 1
        return ast->createNodeValueInt(value.value._bool ? 1 : 0);
      case VALUE_TYPE_INT:
      case VALUE_TYPE_DOUBLE:
        // already numeric!
        return this;
      case VALUE_TYPE_STRING:
        try {
          // try converting string to number
          double v = std::stod(value.value._string);
          return ast->createNodeValueDouble(v);
        }
        catch (...) {
          // conversion failed
        }
        // fall-through intentional
    }
    // fall-through intentional
  }
  else if (type == NODE_TYPE_LIST) {
    size_t const n = numMembers();
    if (n == 0) {
      // [ ] => 0
      return ast->createNodeValueInt(0);
    }
    else if (n == 1) {
      auto member = getMember(0);
      // convert only member to number
      return member->castToNumber(ast);
    }
    // fall-through intentional
  }
  else if (type == NODE_TYPE_ARRAY) {
    // fall-through intentional
  }

  return ast->createNodeValueNull();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert the node's value to a string value
/// this may create a new node or return the node itself if it is already a
/// string value node
////////////////////////////////////////////////////////////////////////////////

AstNode* AstNode::castToString (Ast* ast) {
  TRI_ASSERT(type == NODE_TYPE_VALUE || 
             type == NODE_TYPE_LIST || 
             type == NODE_TYPE_ARRAY);

  if (type == NODE_TYPE_VALUE && value.type == VALUE_TYPE_STRING) {
    // already a string
    return this;
  }

  // stringify node
  triagens::basics::StringBuffer buffer(TRI_UNKNOWN_MEM_ZONE);
  append(&buffer, false);

  char const* value = ast->query()->registerString(buffer.c_str(), buffer.length(), false);
  TRI_ASSERT(value != nullptr);
  return ast->createNodeValueString(value);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a JSON representation of the node to the JSON list specified
/// in the first argument
////////////////////////////////////////////////////////////////////////////////

void AstNode::toJson (TRI_json_t* json,
                      TRI_memory_zone_t* zone,
                      bool verbose) const {
  TRI_ASSERT(TRI_IsListJson(json));

  TRI_json_t* node = toJson(zone, verbose);

  if (node == nullptr) {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
  }

  TRI_PushBack3ListJson(zone, json, node);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief get the integer value of a node, provided the node is a value node
////////////////////////////////////////////////////////////////////////////////

int64_t AstNode::getIntValue () const {
  if (type == NODE_TYPE_VALUE) {
    switch (value.type) {
      case VALUE_TYPE_NULL:
        return 0;
      case VALUE_TYPE_BOOL:
        return (value.value._bool ? 1 : 0);
      case VALUE_TYPE_INT:
        return value.value._int;
      case VALUE_TYPE_DOUBLE:
        return static_cast<int64_t>(value.value._double);
      case VALUE_TYPE_STRING:
        return 0;
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief get the double value of a node, provided the node is a value node
////////////////////////////////////////////////////////////////////////////////

double AstNode::getDoubleValue () const {
  if (type == NODE_TYPE_VALUE) {
    switch (value.type) {
      case VALUE_TYPE_NULL:
        return 0.0;
      case VALUE_TYPE_BOOL:
        return (value.value._bool ? 1.0 : 0.0);
      case VALUE_TYPE_INT:
        return static_cast<double>(value.value._int);
      case VALUE_TYPE_DOUBLE:
        return value.value._double;
      case VALUE_TYPE_STRING:
        return 0.0;
    }
  }
  return 0.0;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not the node value is trueish
////////////////////////////////////////////////////////////////////////////////

bool AstNode::isTrue () const {
  if (type == NODE_TYPE_VALUE) {
    switch (value.type) {
      case VALUE_TYPE_NULL: 
        return false;
      case VALUE_TYPE_BOOL: 
        return value.value._bool;
      case VALUE_TYPE_INT: 
        return (value.value._int != 0);
      case VALUE_TYPE_DOUBLE: 
        return value.value._double != 0.0;
      case VALUE_TYPE_STRING: 
        return (*value.value._string != '\0');
    }
  }
  else if (type == NODE_TYPE_LIST ||
           type == NODE_TYPE_ARRAY) {
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not the node value is falsey
////////////////////////////////////////////////////////////////////////////////

bool AstNode::isFalse () const {
  return ! isTrue();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not a node is simple enough to be used in a simple
/// expression
////////////////////////////////////////////////////////////////////////////////

bool AstNode::isSimple () const {
  if (hasFlag(FLAG_SIMPLE)) {
    // fast track exit
    return true;
  }

  if (type == NODE_TYPE_REFERENCE ||
      type == NODE_TYPE_VALUE) {
    setFlag(FLAG_SIMPLE);
    return true;
  }

  if (type == NODE_TYPE_LIST ||
      type == NODE_TYPE_ARRAY) {
    size_t const n = numMembers();
    for (size_t i = 0; i < n; ++i) {
      auto member = getMember(i);
      if (! member->isSimple()) {
        return false;
      }
    }

    setFlag(FLAG_SIMPLE);
    return true;
  }

  if (type == NODE_TYPE_ARRAY_ELEMENT || 
      type == NODE_TYPE_ATTRIBUTE_ACCESS ||
      type == NODE_TYPE_OPERATOR_UNARY_NOT) {
    TRI_ASSERT(numMembers() == 1);

    if (! getMember(0)->isSimple()) {
      return false;
    }

    setFlag(FLAG_SIMPLE);
    return true;
  }

  if (type == NODE_TYPE_FCALL) {
    // some functions have C++ handlers
    // check if the called function is one of them
    auto func = static_cast<Function*>(getData());
    TRI_ASSERT(func != nullptr);

    if (func->implementation == nullptr || ! getMember(0)->isSimple()) {
      return false;
    }

    setFlag(FLAG_SIMPLE);
    return true;
  }
  
  if (type == NODE_TYPE_OPERATOR_BINARY_AND ||
      type == NODE_TYPE_OPERATOR_BINARY_OR ||
      type == NODE_TYPE_OPERATOR_BINARY_EQ ||
      type == NODE_TYPE_OPERATOR_BINARY_NE ||
      type == NODE_TYPE_OPERATOR_BINARY_LT ||
      type == NODE_TYPE_OPERATOR_BINARY_LE ||
      type == NODE_TYPE_OPERATOR_BINARY_GT ||
      type == NODE_TYPE_OPERATOR_BINARY_GE ||
      type == NODE_TYPE_OPERATOR_BINARY_IN ||
      type == NODE_TYPE_OPERATOR_BINARY_NIN ||
      type == NODE_TYPE_RANGE ||
      type == NODE_TYPE_INDEXED_ACCESS) {
    // a logical operator is simple if its operands are simple
    // a comparison operator is simple if both bounds are simple
    // a range is simple if both bounds are simple
    if (! getMember(0)->isSimple() || ! getMember(1)->isSimple()) {
      return false;
    }
 
    setFlag(FLAG_SIMPLE);
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not a node has a constant value
////////////////////////////////////////////////////////////////////////////////

bool AstNode::isConstant () const {
  if (hasFlag(FLAG_CONSTANT)) {
    // fast track exit
    return true;
  }
  if (hasFlag(FLAG_DYNAMIC)) {
    // fast track exit
    return false;
  }

  if (type == NODE_TYPE_VALUE) {
    setFlag(FLAG_CONSTANT);
    return true;
  }

  if (type == NODE_TYPE_LIST) {
    size_t const n = numMembers();

    for (size_t i = 0; i < n; ++i) {
      auto member = getMember(i);

      if (member != nullptr) {
        if (! member->isConstant()) {
          setFlag(FLAG_DYNAMIC);
          return false;
        }
      }
    }

    setFlag(FLAG_CONSTANT);
    return true;
  }

  if (type == NODE_TYPE_ARRAY) {
    size_t const n = numMembers();

    for (size_t i = 0; i < n; ++i) {
      auto member = getMember(i);

      if (member != nullptr) {
        auto value = member->getMember(0);

        if (value == nullptr) {
          continue;
        }

        if (! value->isConstant()) {
          setFlag(FLAG_DYNAMIC);
          return false;
        }
      }
    }

    setFlag(FLAG_CONSTANT);
    return true;
  }

  setFlag(FLAG_DYNAMIC);
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not a node is a comparison operator
////////////////////////////////////////////////////////////////////////////////

bool AstNode::isComparisonOperator () const {
  return (type == NODE_TYPE_OPERATOR_BINARY_EQ ||
          type == NODE_TYPE_OPERATOR_BINARY_NE ||
          type == NODE_TYPE_OPERATOR_BINARY_LT ||
          type == NODE_TYPE_OPERATOR_BINARY_LE ||
          type == NODE_TYPE_OPERATOR_BINARY_GT ||
          type == NODE_TYPE_OPERATOR_BINARY_GE ||
          type == NODE_TYPE_OPERATOR_BINARY_IN ||
          type == NODE_TYPE_OPERATOR_BINARY_NIN);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not a node (and its subnodes) can throw a runtime
/// exception
////////////////////////////////////////////////////////////////////////////////

bool AstNode::canThrow () const {
  // check sub-nodes first
  size_t const n = numMembers();
  for (size_t i = 0; i < n; ++i) {
    auto member = getMember(i);
    if (member->canThrow()) {
      // if any sub-node may throw, the whole branch may throw
      return true;
    }
  }

  // no sub-node throws, now check ourselves

  if (type == NODE_TYPE_FCALL) {
    auto func = static_cast<Function*>(getData());

    // built-in functions may or may not throw
    // we are currently reporting non-deterministic functions as
    // potentially throwing. This is not correct on the one hand, but on
    // the other hand we must not optimize or move non-deterministic functions
    // during optimization
    // TODO: move the check for isDeterministic into a function of its
    // own and check it from the optimizer rules
    return func->canThrow || ! func->isDeterministic;
  }
  
  if (type == NODE_TYPE_FCALL_USER) {
    // user functions can always throw
    return true;
  }

  // everything else does not throw!
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not a node (and its subnodes) can safely be executed on
/// a DB server
////////////////////////////////////////////////////////////////////////////////

bool AstNode::canRunOnDBServer () const {
  // check sub-nodes first
  size_t const n = numMembers();
  for (size_t i = 0; i < n; ++i) {
    auto member = getMember(i);
    if (! member->canRunOnDBServer()) {
      // if any sub-node cannot run on a DB server, we can't either
      return false;
    }
  }

  // now check ourselves
  if (type == NODE_TYPE_FCALL) {
    // built-in function
    auto func = static_cast<Function*>(getData());
    return func->canRunOnDBServer;
  }
  
  if (type == NODE_TYPE_FCALL_USER) {
    // user function. we don't know anything about it
    return false;
  }

  // everyhing else can run everywhere
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not a node (and its subnodes) is deterministic
////////////////////////////////////////////////////////////////////////////////

bool AstNode::isDeterministic () const {
  // check sub-nodes first
  size_t const n = numMembers();
  for (size_t i = 0; i < n; ++i) {
    auto member = getMember(i);
    if (! member->isDeterministic()) {
      // if any sub-node is non-deterministic, we are neither
      return false;
    }
  }

  if (type == NODE_TYPE_FCALL) {
    // built-in functions may or may not be deterministic
    auto func = static_cast<Function*>(getData());
    return func->isDeterministic;
  }
  
  if (type == NODE_TYPE_FCALL_USER) {
    // user functions are always non-deterministic
    return false;
  }

  // everything else is deterministic
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone a node, recursively
////////////////////////////////////////////////////////////////////////////////

AstNode* AstNode::clone (Ast* ast) const {
  return ast->clone(this);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief append a string representation of the node to a string buffer
////////////////////////////////////////////////////////////////////////////////

void AstNode::append (triagens::basics::StringBuffer* buffer,
                      bool verbose) const {
  if (type == NODE_TYPE_VALUE) {
    appendValue(buffer);
    return;
  }

  if (type == NODE_TYPE_LIST) {
    size_t const n = numMembers();
    if (verbose || n > 0) {
      if (verbose || n > 1) {
        buffer->appendChar('[');
      }
      for (size_t i = 0; i < n; ++i) {
        if (i > 0) {
          buffer->appendChar(',');
        }

        AstNode* member = getMember(i);
        if (member != nullptr) {
          member->append(buffer, verbose);
        }
      }
      if (verbose || n > 1) {
        buffer->appendChar(']');
      }
    }
    return;
  }

  if (type == NODE_TYPE_ARRAY) {
    if (verbose) {
      buffer->appendChar('{');
      size_t const n = numMembers();
      for (size_t i = 0; i < n; ++i) {
        if (i > 0) {
          buffer->appendChar(',');
        }

        AstNode* member = getMember(i);
        if (member != nullptr) {
          TRI_ASSERT(member->type == NODE_TYPE_ARRAY_ELEMENT);
          TRI_ASSERT(member->numMembers() == 1);

          buffer->appendChar('"');
          buffer->appendJsonEncoded(member->getStringValue());
          buffer->appendText("\":", 2);

          member->getMember(0)->append(buffer, verbose);
        }
      }
      buffer->appendChar('}');
    }
    else {
      buffer->appendText("[object Object]");
    }
    return;
  }
    
  if (type == NODE_TYPE_REFERENCE) {
    // not used by V8
    auto variable = static_cast<Variable*>(getData());
    TRI_ASSERT(variable != nullptr);
    // we're intentionally not using the variable name as it is not necessarily
    // unique within a query (hey COLLECT, I am looking at you!)
    buffer->appendChar('$');
    buffer->appendInteger(variable->id);
    return;
  }
    
  if (type == NODE_TYPE_INDEXED_ACCESS) {
    // not used by V8
    auto member = getMember(0);
    auto index = getMember(1);
    member->append(buffer, verbose);
    buffer->appendChar('[');
    index->append(buffer, verbose);
    buffer->appendChar(']');
    return;
  }
    
  if (type == NODE_TYPE_ATTRIBUTE_ACCESS) {
    // not used by V8
    auto member = getMember(0);
    member->append(buffer, verbose);
    buffer->appendChar('.');
    buffer->appendText(getStringValue());
    return;
  }
    
  if (type == NODE_TYPE_FCALL) {
    auto func = static_cast<Function*>(getData());
    buffer->appendText(func->externalName);
    buffer->appendChar('(');
    getMember(0)->append(buffer, verbose);
    buffer->appendChar(')');
    return;
  } 
  
  if (type == NODE_TYPE_OPERATOR_UNARY_NOT ||
      type == NODE_TYPE_OPERATOR_UNARY_PLUS ||
      type == NODE_TYPE_OPERATOR_UNARY_MINUS) {
    TRI_ASSERT(numMembers() == 1);
    auto it = Operators.find(static_cast<int>(type));
    TRI_ASSERT(it != Operators.end());
    buffer->appendChar(' ');
    buffer->appendText((*it).second);

    getMember(0)->append(buffer, verbose);
    return;
  }

  if (type == NODE_TYPE_OPERATOR_BINARY_AND ||
      type == NODE_TYPE_OPERATOR_BINARY_OR ||
      type == NODE_TYPE_OPERATOR_BINARY_PLUS ||
      type == NODE_TYPE_OPERATOR_BINARY_MINUS ||
      type == NODE_TYPE_OPERATOR_BINARY_TIMES ||
      type == NODE_TYPE_OPERATOR_BINARY_DIV ||
      type == NODE_TYPE_OPERATOR_BINARY_MOD ||
      type == NODE_TYPE_OPERATOR_BINARY_EQ ||
      type == NODE_TYPE_OPERATOR_BINARY_NE ||
      type == NODE_TYPE_OPERATOR_BINARY_LT ||
      type == NODE_TYPE_OPERATOR_BINARY_LE ||
      type == NODE_TYPE_OPERATOR_BINARY_GT ||
      type == NODE_TYPE_OPERATOR_BINARY_GE ||
      type == NODE_TYPE_OPERATOR_BINARY_IN ||
      type == NODE_TYPE_OPERATOR_BINARY_NIN) {
    TRI_ASSERT(numMembers() == 2);
    auto it = Operators.find(type);
    TRI_ASSERT(it != Operators.end());

    getMember(0)->append(buffer, verbose);
    buffer->appendChar(' ');
    buffer->appendText((*it).second);
    buffer->appendChar(' ');
    getMember(1)->append(buffer, verbose);
    return;
  }
  
  std::string message("stringification not supported for node type ");
  message.append(getTypeString());
  THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, message);
}

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief stringify the value of a node into a string buffer
/// this method is used when generated JavaScript code for the node!
////////////////////////////////////////////////////////////////////////////////

void AstNode::appendValue (triagens::basics::StringBuffer* buffer) const {
  TRI_ASSERT(type == NODE_TYPE_VALUE);

  switch (value.type) {
    case VALUE_TYPE_BOOL: {
      if (value.value._bool) {
        buffer->appendText("true", 4);
      }
      else {
        buffer->appendText("false", 5);
      }
      break;
    }

    case VALUE_TYPE_INT: {
      buffer->appendInteger(value.value._int);
      break;
    }

    case VALUE_TYPE_DOUBLE: {
      buffer->appendDecimal(value.value._double);
      break;
    }

    case VALUE_TYPE_STRING: {
      buffer->appendChar('"');
      buffer->appendJsonEncoded(value.value._string);
      buffer->appendChar('"');
      break;
    }

    case VALUE_TYPE_NULL: 
    default: {
      buffer->appendText("null", 4);
      break;
    }
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End: