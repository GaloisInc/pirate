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

#include "Annotations.hpp"
#include "indent_facet.hpp"

void MinAnnotation::cDeclareConstraint(std::ostream &ostream, std::string identifier) {
    ostream << "if" << " " << "(" << identifier << " " << "<" << " " << min << ")";
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "return" << " " << "-1" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void MaxAnnotation::cDeclareConstraint(std::ostream &ostream, std::string identifier) {
    ostream << "if" << " " << "(" << identifier << " " << ">" << " " << max << ")";
    ostream << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "return" << " " << "-1" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void RangeAnnotation::cDeclareConstraint(std::ostream &ostream, std::string identifier) {
    ostream << "if" << " " << "(" << "(" << identifier << " " << "<" << " " << min << ")";
    ostream << " " << "||" << " " << "(" << identifier << " " << ">" << " " << max << ")";
    ostream << ")" << " " << "{" << std::endl;
    ostream << indent_manip::push;
    ostream << "return" << " " << "-1" << ";" << std::endl;
    ostream << indent_manip::pop;
    ostream << "}" << std::endl;
}

void RoundAnnotation::cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) {
    CDRTypeOf typeOf = typeSpec->typeOf();
    std::string roundFunc = "";
    switch (typeOf) {
        case CDRTypeOf::FLOAT_T:
            roundFunc = "nearbyintf";
            break;
        case CDRTypeOf::DOUBLE_T:
            roundFunc = "nearbyint";
            break;
        default:
            break;
    }
    if (roundFunc != "") {
        ostream << "int" << " " << "rmode" << " " << "=" << " ";
        ostream << "fegetround" << "(" << ")" << ";" << std::endl;
        ostream << "fesetround" << "(" << "FE_TONEAREST" << ")" << ";" << std::endl;
        ostream << identifier << " " << "=" << " ";
        ostream << roundFunc << "(" << identifier << ")" << ";" << std::endl;
        ostream << "fesetround" << "(" << "rmode" << ")" << ";" << std::endl;
    }
}
