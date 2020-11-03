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

#include "CDRTypes.hpp"

class ValueAnnotation : public AnnotationSpec {
public:
    std::string value;
    ValueAnnotation(int id, std::string value) : AnnotationSpec(id), value(value) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~ValueAnnotation() { };
};

class MinAnnotation : public AnnotationSpec {
public:
    std::string min;
    MinAnnotation(int id, std::string min) : AnnotationSpec(id), min(min) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~MinAnnotation() { };
};

class MaxAnnotation : public AnnotationSpec {
public:
    std::string max;
    MaxAnnotation(int id, std::string max) : AnnotationSpec(id), max(max) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~MaxAnnotation() { };
};

class RangeAnnotation : public AnnotationSpec {
public:
    std::string min;
    std::string max;
    RangeAnnotation(int id, std::string min, std::string max) : AnnotationSpec(id), min(min), max(max) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override;
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~RangeAnnotation() { };
};

class RoundAnnotation : public AnnotationSpec {
public:
    RoundAnnotation(int id) : AnnotationSpec(id) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override { };
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override;
    virtual ~RoundAnnotation() { };
};

class ErrorAnnotation : public AnnotationSpec {
public:
    ErrorAnnotation() : AnnotationSpec(0) { }
    virtual void cDeclareConstraint(std::ostream &ostream, std::string identifier) override { };
    virtual void cDeclareTransform(std::ostream &ostream, TypeSpec *typeSpec, std::string identifier) override { };
    virtual ~ErrorAnnotation() { };
};
