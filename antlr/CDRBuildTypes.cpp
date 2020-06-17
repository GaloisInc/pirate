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
  TypeSpec *typeSpec = new ModuleDecl(ctx->identifier()->getText());
  ModuleDecl *moduleDecl = dynamic_cast<ModuleDecl*>(typeSpec);
  std::vector<IDLParser::DefinitionContext*> definitions = ctx->definition();
  for (IDLParser::DefinitionContext* definitionCtx : definitions) {
    moduleDecl->addDefinition(definitionCtx->accept(this));
  }
  return typeSpec;
}

antlrcpp::Any CDRBuildTypes::visitStruct_type(IDLParser::Struct_typeContext *ctx) {
  TypeSpec *typeSpec = new StructTypeSpec(ctx->identifier()->getText());
  StructTypeSpec *structSpec = dynamic_cast<StructTypeSpec*>(typeSpec);
  std::vector<IDLParser::MemberContext*> members = ctx->member_list()->member();
  for (IDLParser::MemberContext* memberCtx : members) {
    structSpec->addMember(memberCtx->accept(this));
  }
  return typeSpec;
}

antlrcpp::Any CDRBuildTypes::visitMember(IDLParser::MemberContext *ctx) {
  TypeSpec* typeSpec = ctx->type_spec()->accept(this);
  StructMember* structMember = new StructMember(typeSpec);
  std::vector<IDLParser::DeclaratorContext*> declCtxs = ctx->declarators()->declarator();
  for (IDLParser::DeclaratorContext* declCtx : declCtxs) {
    Declarator* decl = declCtx->accept(this);
    structMember->addDeclarator(decl);
  }
  return structMember;
}

antlrcpp::Any CDRBuildTypes::visitSimple_declarator(IDLParser::Simple_declaratorContext *ctx) {
  return new Declarator(ctx->ID()->getText());
}

antlrcpp::Any CDRBuildTypes::visitArray_declarator(IDLParser::Array_declaratorContext *ctx) {
  // TODO: implement multidimensional arrays
  int len = atoi(ctx->fixed_array_size()[0]->positive_int_const()->getText().c_str());
  return new Declarator(ctx->ID()->getText(), len);
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
