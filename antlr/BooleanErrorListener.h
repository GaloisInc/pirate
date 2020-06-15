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
