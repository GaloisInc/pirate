/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include "antlr4-runtime.h"
#include "CDRBuildTypes.h"
#include "CDRTypes.h"

#include <algorithm>

antlrcpp::Any CDRBuildTypes::aggregateResult(antlrcpp::Any aggregate, const antlrcpp::Any &nextResult) {
  if (aggregate.isNotNull() && nextResult.isNotNull()) {
    throw std::runtime_error("unexpected aggregation");
  } else if (aggregate.isNotNull()) {
    return aggregate;
  } else if (nextResult.isNotNull()) {
    return nextResult;
  } else {
    return nullptr;
  }
}

antlrcpp::Any CDRBuildTypes::visitModule(IDLParser::ModuleContext *ctx) {
  std::string identifier = ctx->identifier()->getText();
  transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
  TypeSpec *typeSpec = new ModuleDecl(identifier);
  ModuleDecl *moduleDecl = dynamic_cast<ModuleDecl*>(typeSpec);
  std::vector<IDLParser::DefinitionContext*> definitions = ctx->definition();
  for (IDLParser::DefinitionContext* definitionCtx : definitions) {
    moduleDecl->addDefinition(definitionCtx->accept(this));
  }
  return typeSpec;
}

static std::string unknownMember(std::string name, IDLParser::Annotation_appl_paramContext* param) {
  return "unknown annotation member " + param->ID()->getText() +
    " for annotation @" + name + " on line " + std::to_string(param->getStart()->getLine());
}

static std::string unknownDefaultMember(std::string name, IDLParser::Annotation_appl_paramsContext *params) {
  return "unknown default annotation member for annotation @" + name +
    " on line " + std::to_string(params->getStart()->getLine());
}

static std::string missingRequiredMember(std::string name, IDLParser::Annotation_appl_paramsContext *params) {
  return "missing required annotation members for annotation @" + name +
    " on line " + std::to_string(params->getStart()->getLine());
}

MinAnnotation *CDRBuildTypes::buildMinAnnotation(IDLParser::Annotation_appl_paramsContext *params) {
  std::string min = "";
  bool error = false;
  if (params->const_exp() != nullptr) {
    min = params->const_exp()->getText();
  } else {
    for (IDLParser::Annotation_appl_paramContext* param : params->annotation_appl_param()) {
      if (param->ID()->getText() == "value") {
        min = param->const_exp()->getText();
      } else {
        errors.insert(unknownMember("min", param));
        error = true;
      }
    }
  }
  if (min.length() > 0) {
    return new MinAnnotation(++annotationIds, min);
  } else if (!error) {
    errors.insert(missingRequiredMember("min", params));
  }
  return nullptr;
}

MaxAnnotation *CDRBuildTypes::buildMaxAnnotation(IDLParser::Annotation_appl_paramsContext *params) {
  std::string max = "";
  bool error = false;
  if (params->const_exp() != nullptr) {
    max = params->const_exp()->getText();
  } else {
    for (IDLParser::Annotation_appl_paramContext* param : params->annotation_appl_param()) {
      if (param->ID()->getText() == "value") {
        max = param->const_exp()->getText();
      } else {
        errors.insert(unknownMember("max", param));
        error = true;
      }
    }
  }
  if (max.length() > 0) {
    return new MaxAnnotation(++annotationIds, max);
  } else if (!error) {
    errors.insert(missingRequiredMember("max", params));
  }
  return nullptr;
}

RangeAnnotation *CDRBuildTypes::buildRangeAnnotation(IDLParser::Annotation_appl_paramsContext *params) {
  std::string min = "";
  std::string max = "";
  bool error = false;
  if (params->const_exp() != nullptr) {
    // do nothing
  } else {
    for (IDLParser::Annotation_appl_paramContext* param : params->annotation_appl_param()) {
      if (param->ID()->getText() == "min") {
        min = param->const_exp()->getText();
      } else if (param->ID()->getText() == "max") {
        max = param->const_exp()->getText();
      } else {
        errors.insert(unknownMember("range", param));
        error = true;
      }
    }
  }
  if ((min.length() > 0) && (max.length() > 0)) {
    return new RangeAnnotation(++annotationIds, min, max);
  } else if (!error) {
    errors.insert(missingRequiredMember("range", params));
  }
  return nullptr;
}

RoundAnnotation *CDRBuildTypes::buildRoundAnnotation(IDLParser::Annotation_appl_paramsContext *params) {
  bool error = false;
  if (params != nullptr) {
    if (params->const_exp() != nullptr) {
      errors.insert(unknownDefaultMember("round", params));
      error = true;
    } else {
      for (IDLParser::Annotation_appl_paramContext* param : params->annotation_appl_param()) {
        errors.insert(unknownMember("round", param));
        error = true;
      }
    }
  }
  if (!error) {
    return new RoundAnnotation(++annotationIds);
  } else {
    return nullptr;
  }
}

antlrcpp::Any CDRBuildTypes::visitAnnotation_appl(IDLParser::Annotation_applContext *ctx) {
  std::string name = ctx->scoped_name()->getText();
  IDLParser::Annotation_appl_paramsContext *params = ctx->annotation_appl_params();
  AnnotationSpec *annotationSpec = nullptr;
  if (name == "min") {
    annotationSpec = buildMinAnnotation(params);
  } else if (name == "max") {
    annotationSpec = buildMaxAnnotation(params);
  } else if (name == "range") {
    annotationSpec = buildRangeAnnotation(params);
  } else if (name == "round") {
    annotationSpec = buildRoundAnnotation(params);
  } else {
    errors.insert("unknown annotation @" + name +
      " on line " + std::to_string(ctx->getStart()->getLine()));
  }
  if (annotationSpec == nullptr) {
    annotationSpec = new ErrorAnnotation();
  }
  return annotationSpec;
}

antlrcpp::Any CDRBuildTypes::visitSimple_type_spec(IDLParser::Simple_type_specContext *ctx) {
  if (ctx->scoped_name() != nullptr) {
    std::string name = ctx->scoped_name()->getText();
    TypeSpec *typeSpecRef = typeDeclarations[name];
    if (typeSpecRef == nullptr) {
      errors.insert("unknown reference to type " + name + " on line " +
        std::to_string(ctx->scoped_name()->getStart()->getLine()));
      return BaseTypeSpec::errorType();
    } else {
      TypeSpec *typeSpec = new TypeReference(typeSpecRef);
      return typeSpec;
    }
  } else {
    return ctx->base_type_spec()->accept(this);
  }
}

antlrcpp::Any CDRBuildTypes::visitEnum_type(IDLParser::Enum_typeContext *ctx) {
  std::string identifier = ctx->identifier()->getText();
  transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
  TypeSpec *typeSpec = new EnumTypeSpec(identifier);
  EnumTypeSpec *enumSpec = dynamic_cast<EnumTypeSpec*>(typeSpec);
  std::vector<IDLParser::EnumeratorContext *> enumerators = ctx->enumerator();
  for (IDLParser::EnumeratorContext* enumCtx : enumerators) {
    enumSpec->addEnumerator(enumCtx->identifier()->getText());
  }
  typeDeclarations[ctx->identifier()->getText()] = typeSpec;
  return typeSpec;
}

antlrcpp::Any CDRBuildTypes::visitStruct_type(IDLParser::Struct_typeContext *ctx) {
  std::string identifier = ctx->identifier()->getText();
  transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
  TypeSpec *typeSpec = new StructTypeSpec(identifier);
  StructTypeSpec *structSpec = dynamic_cast<StructTypeSpec*>(typeSpec);
  std::vector<IDLParser::MemberContext*> members = ctx->member_list()->member();
  for (IDLParser::MemberContext* memberCtx : members) {
    structSpec->addMember(memberCtx->accept(this));
  }
  typeDeclarations[ctx->identifier()->getText()] = typeSpec;
  return typeSpec;
}

antlrcpp::Any CDRBuildTypes::visitMember(IDLParser::MemberContext *ctx) {
  TypeSpec* typeSpec = ctx->type_spec()->accept(this);
  StructMember* structMember = new StructMember(typeSpec);
  std::vector<IDLParser::DeclaratorContext*> declCtxs = ctx->declarators()->declarator();
  for (IDLParser::DeclaratorContext* declCtx : declCtxs) {
    Declarator* decl = declCtx->accept(this);
    if (ctx->annapps() != nullptr) {
      std::vector<IDLParser::Annotation_applContext*> annCtxs = ctx->annapps()->annotation_appl();
      for (IDLParser::Annotation_applContext* annCtx : annCtxs) {
        decl->addAnnotation(annCtx->accept(this));
      }
    }
    structMember->addDeclarator(decl);
  }

  return structMember;
}

antlrcpp::Any CDRBuildTypes::visitUnion_type(IDLParser::Union_typeContext *ctx) {
  std::string identifier = ctx->identifier()->getText();
  transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
  TypeSpec *switchType = ctx->switch_type_spec()->accept(this);
  TypeSpec *typeSpec = new UnionTypeSpec(identifier, switchType);
  UnionTypeSpec *unionSpec = dynamic_cast<UnionTypeSpec*>(typeSpec);
  std::vector<IDLParser::Case_stmtContext*> members = ctx->switch_body()->case_stmt();
  for (IDLParser::Case_stmtContext* caseCtx : members) {
    UnionMember* member = caseCtx->accept(this);
    unionSpec->addMember(member);
  }
  typeDeclarations[ctx->identifier()->getText()] = typeSpec;
  return typeSpec;
}

antlrcpp::Any CDRBuildTypes::visitCase_stmt(IDLParser::Case_stmtContext *ctx) {
  TypeSpec* typeSpec = ctx->element_spec()->type_spec()->accept(this);
  Declarator* decl = ctx->element_spec()->declarator()->accept(this);
  UnionMember *member = new UnionMember(typeSpec, decl);
  std::vector<IDLParser::Case_labelContext*> labels = ctx->case_label();
  for (IDLParser::Case_labelContext* labelCtx : labels) {
    if (labelCtx->const_exp() == nullptr) {
      member->setHasDefault();
    } else {
      member->addLabel(labelCtx->const_exp()->getText());
    }
  }
  if (ctx->element_spec()->annapps() != nullptr) {
    std::vector<IDLParser::Annotation_applContext*> annCtxs = ctx->element_spec()->annapps()->annotation_appl();
    for (IDLParser::Annotation_applContext* annCtx : annCtxs) {
      decl->addAnnotation(annCtx->accept(this));
    }
  }
  return member;
}

antlrcpp::Any CDRBuildTypes::visitSimple_declarator(IDLParser::Simple_declaratorContext *ctx) {
  return new Declarator(ctx->ID()->getText());
}

antlrcpp::Any CDRBuildTypes::visitArray_declarator(IDLParser::Array_declaratorContext *ctx) {
  Declarator *decl = new Declarator(ctx->ID()->getText());
  std::vector<IDLParser::Fixed_array_sizeContext*> sizes = ctx->fixed_array_size();
  for (IDLParser::Fixed_array_sizeContext* sizeCtx : sizes) {
    int dim = atoi(sizeCtx->positive_int_const()->getText().c_str());
    decl->addDimension(dim);
  }
  return decl;
}

antlrcpp::Any CDRBuildTypes::visitFloating_pt_type(IDLParser::Floating_pt_typeContext *ctx) {
  if (ctx->KW_LONG() && ctx->KW_DOUBLE()) {
    return BaseTypeSpec::longDoubleType();
  } else if (ctx->KW_DOUBLE()) {
    return BaseTypeSpec::doubleType();
  } else if (ctx->KW_FLOAT()) {
    return BaseTypeSpec::floatType();
  } else {
    return nullptr;
  }
}

antlrcpp::Any CDRBuildTypes::visitSigned_tiny_int(IDLParser::Signed_tiny_intContext *ctx) {
  return BaseTypeSpec::tinyType();
}

antlrcpp::Any CDRBuildTypes::visitSigned_short_int(IDLParser::Signed_short_intContext *ctx) {
  return BaseTypeSpec::shortType();
}

antlrcpp::Any CDRBuildTypes::visitSigned_long_int(IDLParser::Signed_long_intContext *ctx) {
  return BaseTypeSpec::longType();
}

antlrcpp::Any CDRBuildTypes::visitSigned_longlong_int(IDLParser::Signed_longlong_intContext *ctx) {
  return BaseTypeSpec::longLongType();
}

antlrcpp::Any CDRBuildTypes::visitUnsigned_tiny_int(IDLParser::Unsigned_tiny_intContext *ctx) {
  return BaseTypeSpec::unsignedTinyType();
}

antlrcpp::Any CDRBuildTypes::visitUnsigned_short_int(IDLParser::Unsigned_short_intContext *ctx) {
  return BaseTypeSpec::unsignedShortType();
}

antlrcpp::Any CDRBuildTypes::visitUnsigned_long_int(IDLParser::Unsigned_long_intContext *ctx) {
  return BaseTypeSpec::unsignedLongType();
}

antlrcpp::Any CDRBuildTypes::visitUnsigned_longlong_int(IDLParser::Unsigned_longlong_intContext *ctx) {
  return BaseTypeSpec::unsignedLongLongType();
}

antlrcpp::Any CDRBuildTypes::visitChar_type(IDLParser::Char_typeContext *ctx) {
  return BaseTypeSpec::charType();
}

antlrcpp::Any CDRBuildTypes::visitBoolean_type(IDLParser::Boolean_typeContext *ctx) {
  return BaseTypeSpec::boolType();
}

antlrcpp::Any CDRBuildTypes::visitOctet_type(IDLParser::Octet_typeContext *ctx) {
  return BaseTypeSpec::octetType();
}
