#pragma once

#include <src/ast.h>

namespace v8 {
  namespace internal {

class PrettyPrinter: public AstVisitor {
public:
  PrettyPrinter(Zone* zone);
  virtual ~PrettyPrinter();

  // The following routines print a node into a string.
  // The result string is alive as long as the PrettyPrinter is alive.
  const char* Print(AstNode* node);
  const char* PrintExpression(FunctionLiteral* program);
  const char* PrintProgram(FunctionLiteral* program);

  void Print(const char* format, ...);

  // Print a node to stdout.
  static void PrintOut(Zone *zone, AstNode* node);

  // Individual nodes
#define DECLARE_VISIT(type) virtual void Visit##type(type* node);
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT

private:
  char* output_;  // output string buffer
  int size_;  // output_ size
  int pos_;  // current printing position

protected:
  void Init();
  const char* Output() const { return output_; }

  virtual void PrintStatements(ZoneList<Statement*>* statements);
  void PrintLabels(ZoneStringList* labels);
  virtual void PrintArguments(ZoneList<Expression*>* arguments);
  void PrintLiteral(Handle<Object> value, bool quote);
  void PrintParameters(Scope* scope);
  void PrintDeclarations(ZoneList<Declaration*>* declarations);
  void PrintFunctionLiteral(FunctionLiteral* function);
  void PrintCaseClause(CaseClause* clause);

  DEFINE_AST_VISITOR_SUBCLASS_MEMBERS();
};


// Prints the AST structure
class AstPrinter: public PrettyPrinter {
public:
  AstPrinter(Zone* zone);
  virtual ~AstPrinter();

  const char* PrintProgram(FunctionLiteral* program);

  // Individual nodes
#define DECLARE_VISIT(type) virtual void Visit##type(type* node);
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT

private:
  friend class IndentedScope;
  void PrintIndented(const char* txt);
  void PrintIndentedVisit(const char* s, AstNode* node);

  void PrintStatements(ZoneList<Statement*>* statements);
  void PrintDeclarations(ZoneList<Declaration*>* declarations);
  void PrintParameters(Scope* scope);
  void PrintArguments(ZoneList<Expression*>* arguments);
  void PrintCaseClause(CaseClause* clause);
  void PrintLiteralIndented(const char* info, Handle<Object> value, bool quote);
  void PrintLiteralWithModeIndented(const char* info,
    Variable* var,
    Handle<Object> value);
  void PrintLabelsIndented(ZoneStringList* labels);

  void inc_indent() { indent_++; }
  void dec_indent() { indent_--; }

  int indent_;
};


// Forward declaration of helper classes.
class TagScope;
class AttributesScope;

// Build a C string containing a JSON representation of a function's
// AST. The representation is based on JsonML (www.jsonml.org).
class JsonAstBuilder: public PrettyPrinter {
public:
  JsonAstBuilder(Zone* zone)
    : PrettyPrinter(zone), indent_(0), top_tag_scope_(NULL), attributes_scope_(NULL) {
  }
  virtual ~JsonAstBuilder() {}

  // Controls the indentation of subsequent lines of a tag body after
  // the first line.
  static const int kTagIndentSize = 2;

  // Controls the indentation of subsequent lines of an attributes
  // blocks's body after the first line.
  static const int kAttributesIndentSize = 1;

  // Construct a JSON representation of a function literal.
  const char* BuildProgram(FunctionLiteral* program);

  // Print text indented by the current indentation level.
  void PrintIndented(const char* text) { Print("%*s%s", indent_, "", text); }

  // Change the indentation level.
  void increase_indent(int amount) { indent_ += amount; }
  void decrease_indent(int amount) { indent_ -= amount; }

  // The builder maintains a stack of opened AST node constructors.
  // Each node constructor corresponds to a JsonML tag.
  TagScope* tag() { return top_tag_scope_; }
  void set_tag(TagScope* scope) { top_tag_scope_ = scope; }

  // The builder maintains a pointer to the currently opened attributes
  // of current AST node or NULL if the attributes are not opened.
  AttributesScope* attributes() { return attributes_scope_; }
  void set_attributes(AttributesScope* scope) { attributes_scope_ = scope; }

  // Add an attribute to the currently opened attributes.
  void AddAttribute(const char* name, Handle<String> value);
  void AddAttribute(const char* name, const char* value);
  void AddAttribute(const char* name, int value);
  void AddAttribute(const char* name, bool value);

  // AST node visit functions.
#define DECLARE_VISIT(type) virtual void Visit##type(type* node);
  AST_NODE_LIST(DECLARE_VISIT)
#undef DECLARE_VISIT

private:
  int indent_;
  TagScope* top_tag_scope_;
  AttributesScope* attributes_scope_;

  // Utility function used by AddAttribute implementations.
  void AddAttributePrefix(const char* name);
};


// The JSON AST builder keeps a stack of open element tags (AST node
// constructors from the current iteration point to the root of the
// AST).  TagScope is a helper class to manage the opening and closing
// of tags, the indentation of their bodies, and comma separating their
// contents.
class TagScope BASE_EMBEDDED {
public:
  TagScope(JsonAstBuilder* builder, const char* name);
  ~TagScope();

  void use() { has_body_ = true; }

private:
  JsonAstBuilder* builder_;
  TagScope* next_;
  bool has_body_;
};


// AttributesScope is a helper class to manage the opening and closing
// of attribute blocks, the indentation of their bodies, and comma
// separating their contents. JsonAstBuilder::AddAttribute adds an
// attribute to the currently open AttributesScope. They cannot be
// nested so the builder keeps an optional single scope rather than a
// stack.
class AttributesScope BASE_EMBEDDED {
public:
  explicit AttributesScope(JsonAstBuilder* builder);
  ~AttributesScope();

  bool is_used() { return attribute_count_ > 0; }
  void use() { ++attribute_count_; }

private:
  JsonAstBuilder* builder_;
  int attribute_count_;
};

}}
