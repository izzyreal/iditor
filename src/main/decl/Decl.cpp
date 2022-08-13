#include "Decl.h"

#include <utility>

Decl::Decl(DeclType _declType, std::string &_name_space)
: declType(_declType), name_space(_name_space)
{
}

DeclType Decl::getType()
{
  return declType;
}

std::string &Decl::getNamespace()
{
  return name_space;
}

std::string &TypeDefDecl::getTypeTypeID()
{
  return typeTypeID;
}

TypeDefDecl::TypeDefDecl(
    std::string &name_space,
    std::string _typeTypeID,
    std::string _declaratorTypeID
)
    : Decl(TYPE_DEFINITION, name_space),
      typeTypeID(std::move(_typeTypeID)),
      declaratorTypeID(std::move(_declaratorTypeID))
{
}

std::string &TypeDefDecl::getDeclaratorTypeID()
{
  return declaratorTypeID;
}

std::string &TypeDefDecl::getName()
{
  return typeTypeID;
}

ClassSpecDecl::ClassSpecDecl(std::string &name_space, std::string _name)
    : Decl(CLASS_SPECIFIER, name_space),
      name(std::move(_name))
{
}

std::string &ClassSpecDecl::getName()
{
  return name;
}

FuncDeclaratorDecl::FuncDeclaratorDecl(std::string &name_space, std::string _name)
    : Decl(FUNCTION_DECLARATOR, name_space),
      name(std::move(_name))
{
}

std::string &FuncDeclaratorDecl::getName()
{
  return name;
}
