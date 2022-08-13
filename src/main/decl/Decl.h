#pragma once

#include <string>

enum DeclType
{
  TYPE_DEFINITION,
  CLASS_SPECIFIER,
  FUNCTION_DECLARATOR
};

class Decl
{
public:
  DeclType getType();
  std::string &getNamespace();

  virtual std::string &getName() = 0;

protected:
  explicit Decl(DeclType _declType, std::string &name_space);

private:
  DeclType declType;
  std::string name_space;
};

class TypeDefDecl : public Decl
{
public:
  TypeDefDecl(
      std::string &name_space,
      std::string typeTypeID,
      std::string declaratorTypeID
  );

  std::string &getName() override;

  std::string &getTypeTypeID();
  std::string &getDeclaratorTypeID();

private:
  std::string typeTypeID;
  std::string declaratorTypeID;
};

class ClassSpecDecl : public Decl
{
public:
  ClassSpecDecl(
      std::string &name_space,
      std::string name
  );

  std::string &getName() override;

private:
  std::string name;
};

class FuncDeclaratorDecl : public Decl
{
public:
  FuncDeclaratorDecl(
      std::string &name_space,
      std::string name
  );

  std::string &getName() override;

private:
  std::string name;
};