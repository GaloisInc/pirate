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

#pragma once

#include "BaseErrorListener.h"

class BooleanErrorListener : public antlr4::BaseErrorListener {
private:
  bool error;
public:
  BooleanErrorListener() : error(false) { }

  virtual void syntaxError(antlr4::Recognizer *recognizer,
    antlr4::Token *offendingSymbol, size_t line, size_t charPositionInLine,
    const std::string &msg, std::exception_ptr e) override {
      error = true;
  };

  bool hasError() {
    return error;
  }
};
