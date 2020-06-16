#pragma once

#include "IDLBaseListener.h"

class CDRModuleCounter : public IDLBaseListener {
public:
    CDRModuleCounter() : counter(0) { }
    virtual void enterModule(IDLParser::ModuleContext* ctx) override { counter++; }
    int getCounter() { return counter; }
private:
    int counter;
};
