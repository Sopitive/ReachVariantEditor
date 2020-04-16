#include "compiler.h"
#include "namespaces.h"
#include "../../../helpers/qt/string.h"
#include "../opcode_arg_types/all_indices.h" // OpcodeArgValueTrigger

namespace {
   constexpr char* ce_assignment_operator = "=";
   bool _is_assignment_operator(QString s) {
      constexpr char* operators[] = {
         "=",
         "+=",
         "-=",
         "*=",
         "/=",
         "%=",
         ">>=",
         "<<=",
         ">>>=",
         "~=",
         "^=",
         "&=",
         "|="
      };
      for (size_t i = 0; i < std::extent<decltype(operators)>::value; i++)
         if (s == operators[i])
            return true;
      return false;
   }
   bool _is_comparison_operator(QString s) {
      constexpr char* operators[] = {
         "==",
         "!=",
         ">=",
         "<=",
         ">",
         "<"
      };
      for (size_t i = 0; i < std::extent<decltype(operators)>::value; i++)
         if (s == operators[i])
            return true;
      return false;
   }
   const Megalo::ActionFunction& _get_assignment_opcode() {
      using namespace Megalo;
      static const ActionFunction* result = nullptr;
      if (result)
         return *result;
      //
      for (auto& opcode : actionFunctionList) {
         auto& mapping = opcode.mapping;
         if (mapping.type == OpcodeFuncToScriptMapping::mapping_type::assign) {
            result = &opcode;
            return *result;
         }
      }
      assert(false && "Where is the assignment opcode?");
      __assume(0); // tell MSVC this is unreachable
   }
   const Megalo::ConditionFunction& _get_comparison_opcode() {
      using namespace Megalo;
      static const ConditionFunction* result = nullptr;
      if (result)
         return *result;
      //
      for (auto& opcode : conditionFunctionList) {
         auto& mapping = opcode.mapping;
         if (mapping.type == OpcodeFuncToScriptMapping::mapping_type::compare) {
            result = &opcode;
            return *result;
         }
      }
      assert(false && "Where is the comparison opcode?");
      __assume(0); // tell MSVC this is unreachable
   }
   //
   Megalo::block_type _block_type_to_trigger_type(Megalo::Script::Block::Type type) {
      using namespace Megalo;
      using namespace Megalo::Script;
      switch (type) {
         case Block::Type::basic:
         case Block::Type::if_block:
         case Block::Type::elseif_block:
         case Block::Type::else_block:
            break;
         case Block::Type::for_each_object:
            return block_type::for_each_object;
            break;
         case Block::Type::for_each_object_with_label:
            return block_type::for_each_object_with_label;
            break;
         case Block::Type::for_each_player:
            return block_type::for_each_player;
            break;
         case Block::Type::for_each_player_randomly:
            return block_type::for_each_player_randomly;
            break;
         case Block::Type::for_each_team:
            return block_type::for_each_team;
            break;
      }
      return block_type::normal;
   }
   Megalo::entry_type _block_event_to_trigger_entry(Megalo::Script::Block::Event type) {
      using namespace Megalo;
      using namespace Megalo::Script;
      switch (type) {
         case Block::Event::double_host_migration:
         case Block::Event::host_migration:
            return Megalo::entry_type::on_host_migration;
         case Block::Event::init:
            return Megalo::entry_type::on_init;
         case Block::Event::local:
            return Megalo::entry_type::local;
         case Block::Event::local_init:
            return Megalo::entry_type::on_local_init;
         case Block::Event::object_death:
            return Megalo::entry_type::on_object_death;
         case Block::Event::pregame:
            return Megalo::entry_type::pregame;
      }
      return entry_type::normal;
   }
}
namespace Megalo {
   namespace Script {
      Block::~Block() {
         this->clear();
      }
      void Block::insert_condition(ParsedItem* item) {
         this->conditions.push_back(item);
         //
         // TODO: handling for joiners i.e. "and", "or"
         //
         item->owner = this;
      }
      void Block::insert_item(ParsedItem* item) {
         this->items.push_back(item);
         item->parent = this;
      }
      ParsedItem* Block::item(int32_t i) {
         if (i < 0) {
            i = this->items.size() + i;
            if (i < 0)
               return nullptr;
         } else if (i >= this->items.size())
            return nullptr;
         return this->items[i];
      }
      //
      void Block::clear() {
         for (auto item : this->items)
            delete item;
         this->items.clear();
         for (auto condition : this->conditions)
            delete condition;
         this->conditions.clear();
      }
      //
      void Block::_get_effective_items(std::vector<ParsedItem*>& out, bool include_functions) {
         out.clear();
         for (auto item : this->items) {
            if (auto block = dynamic_cast<Block*>(item)) {
               if (!include_functions) {
                  if (block->type == Type::function)
                     continue;
               }
               out.push_back(item);
               continue;
            }
            if (dynamic_cast<Statement*>(item)) {
               out.push_back(item);
               continue;
            }
         }
      }
      bool Block::_is_if_block() const noexcept {
         switch (this->type) {
            case Type::if_block:
            case Type::elseif_block:
            case Type::else_block:
               return true;
         }
         return false;
      }
      void Block::_make_trigger(Compiler& compiler) {
         if (this->type == Type::function) {
            assert(this->trigger && "The Compiler should've given this user-defined-function Block a trigger when it was first opened.");
            this->trigger->blockType = block_type::normal;
            this->trigger->entryType = entry_type::subroutine;
            if (this->event != Event::none)
               this->trigger->entryType = _block_event_to_trigger_entry(this->event);
            //
            std::vector<ParsedItem*> items;
            this->_get_effective_items(items, false);
            if (items.size() == 1) {
               //
               // If the function only contains a single block, then use that block's type. For example, the following 
               // function should compile as a single for-loop trigger:
               //
               //    function example()    -- this Block owns   the Trigger
               //       for each player do -- this Block shares the Trigger
               //          action
               //       end
               //    end
               //
               auto item = dynamic_cast<Block*>(this->items[0]);
               if (item) {
                  item->trigger = this->trigger; // get the inner Block to write into the function's trigger
                  this->trigger->blockType = _block_type_to_trigger_type(item->type);
               }
            }
            return;
         }
         if (this->trigger)
            //
            // If we already have a trigger, then we're meant to compile our content directly into that 
            // trigger. This will generally be the case for things like the last if-block inside of a 
            // trigger:
            //
            //    for each player do
            //       action
            //       if condition then -- not the last thing in the block, so needs its own Trigger
            //          action
            //       end
            //       if condition then -- the last thing in the block, so writes into parent block's Trigger
            //          action
            //       end
            //    end
            //
            return;
         //
         auto t = this->trigger = new Trigger;
         compiler.results.triggers.push_back(t);
         if (this->parent)
            t->entryType = entry_type::subroutine;
         //
         if (this->event != Event::none) {
            t->entryType = _block_event_to_trigger_entry(this->event);
         }
         switch (this->type) {
            case Type::basic:
            case Type::if_block:
            case Type::elseif_block:
            case Type::else_block:
               break;
            case Type::for_each_object:
               t->blockType = block_type::for_each_object;
               break;
            case Type::for_each_object_with_label:
               t->blockType = block_type::for_each_object_with_label;
               break;
            case Type::for_each_player:
               t->blockType = block_type::for_each_player;
               break;
            case Type::for_each_player_randomly:
               t->blockType = block_type::for_each_player_randomly;
               break;
            case Type::for_each_team:
               t->blockType = block_type::for_each_team;
               break;
         }
      }
      void Block::compile(Compiler& compiler) {
         std::vector<ParsedItem*> items;
         this->_get_effective_items(items);
         if (!items.size()) {
            return;
         }
         //
         this->_make_trigger(compiler);
         {
            auto& count = this->trigger->raw.conditionCount; // multiple Blocks can share one Trigger, so let's co-opt this field to keep track of the or-group
            for (auto item : this->conditions) {
               #if _DEBUG
                  assert(this->type != Type::root && "The root block shouldn't contain any conditions!");
               #endif
               auto cmp = dynamic_cast<Comparison*>(item);
               assert(cmp);
               auto cnd = dynamic_cast<Condition*>(cmp->opcode);
               assert(cnd);
               #if _DEBUG
                  for (auto arg : cnd->arguments)
                     assert(arg && "A compiled condition is missing an argument!");
               #endif
               this->trigger->opcodes.push_back(cnd);
               //
               // Handle or-groups:
               //
               cnd->or_group = count;
               if (!cmp->next_is_or)
                  count += 1;
            }
         }
         std::vector<ParsedItem*> items;
         this->_get_effective_items(items);
         size_t size = items.size();
         for (size_t i = 0; i < size; i++) {
            bool is_last = (i == size - 1);
            auto item    = items[i];
            auto block   = dynamic_cast<Block*>(item);
            if (block) {
               if (is_last && block->_is_if_block())
                  block->trigger = this->trigger;
               block->compile(compiler);
               if (block->trigger != this->trigger) {
                  //
                  // Create a "call nested trigger" opcode.
                  //
                  auto call  = new Action;
                  this->trigger->opcodes.push_back(call);
                  call->function = &actionFunction_runNestedTrigger;
                  auto arg   = (call->function->arguments[0].typeinfo.factory)();
                  call->arguments.push_back(arg);
                  auto arg_c = dynamic_cast<OpcodeArgValueTrigger*>(arg);
                  assert(arg_c && "The argument to the ''run nested trigger'' opcode isn't OpcodeArgValueTrigger anymore? Did someone change the opcode-base?");
                  arg_c->value = compiler._index_of_trigger(block->trigger);
                  assert(arg_c->value >= 0 && "Nested block trigger isn't in the Compiler's trigger list?!");
               }
               continue;
            }
            auto statement = dynamic_cast<Statement*>(item);
            if (statement) {
               assert(statement->opcode);
               #if _DEBUG
                  for (auto arg : statement->opcode->arguments)
                     assert(arg && "A compiled opcode is missing an argument!");
               #endif
               this->trigger->opcodes.push_back(statement->opcode);
               continue;
            }
         }
      }
      //
      Statement::~Statement() {
         if (this->lhs) {
            delete this->lhs;
            this->lhs = nullptr;
         }
         if (this->rhs) {
            delete this->rhs;
            this->rhs = nullptr;
         }
      }
   }
   //
   Compiler::~Compiler() {
      if (!this->results.success) {
         for (auto trigger : this->results.triggers)
            delete trigger;
      }
      this->results.triggers.clear();
      //
      if (this->block && this->block != this->root) {
         delete this->block;
         this->block = nullptr;
      }
      if (this->root) {
         delete this->root;
         this->root = nullptr;
      }
      if (auto statement = this->assignment) {
         if (!statement->owner && !statement->parent) // only free the memory if it wasn't appended to a Block; otherwise, the memory should already be freed
            delete statement;
         this->assignment = nullptr;
      }
      if (auto statement = this->comparison) {
         if (!statement->owner && !statement->parent) // only free the memory if it wasn't appended to a Block; otherwise, the memory should already be freed
            delete statement;
         this->comparison = nullptr;
      }
      this->aliases_in_scope.clear(); // don't free contents; every Alias should have been inside of a Block and freed by that Block
      this->functions_in_scope.clear();
   }
   //
   /*static*/ bool Compiler::is_keyword(QString s) {
      s = s.toLower();
      if (s == "alias") // declare an alias
         return true;
      if (s == "and") // bridge conditions
         return true;
      if (s == "declare") // declare a variable
         return true;
      if (s == "do") // open a generic block
         return true;
      if (s == "else") // close an if- or elseif-block and open a new block
         return true;
      if (s == "elseif") // close an if- or elseif-block and open a new block with conditions
         return true;
      if (s == "end") // close a block
         return true;
      if (s == "for") // open a for loop block
         return true;
      if (s == "function") // open a function block
         return true;
      if (s == "if") // open a new block with conditions
         return true;
      if (s == "not") // indicate that the next condition should be negated
         return true;
      if (s == "on") // designate the event handler type of the next top-level block
         return true;
      if (s == "or") // bridge conditions
         return true;
      if (s == "then") // close an if- or elseif-statement's conditions
         return true;
      return false;
   }
   //
   Script::Alias* Compiler::lookup_relative_alias(QString name, const OpcodeArgTypeinfo* relative_to) {
      auto& list = this->aliases_in_scope;
      size_t size = list.size();
      if (!size)
         return nullptr;
      //
      // Search the list in reverse order, so that newer aliases shadow older ones, and aliases in an 
      // inner scope shadow aliases in an outer scope.
      //
      for (signed int i = size - 1; i >= 0; --i) {
         auto alias = list[i];
         if (!alias->is_relative_alias())
            continue;
         if (alias->get_basis_type() != relative_to)
            continue;
         if (name.compare(alias->name, Qt::CaseInsensitive) == 0)
            return alias;
      }
      return nullptr;
   }
   Script::Alias* Compiler::lookup_absolute_alias(QString name) {
      auto&  list = this->aliases_in_scope;
      size_t size = list.size();
      if (!size)
         return nullptr;
      //
      // Search the list in reverse order, so that newer aliases shadow older ones, and aliases in an 
      // inner scope shadow aliases in an outer scope.
      //
      for (signed int i = size - 1; i >= 0; --i) {
         auto alias = list[i];
         if (alias->is_relative_alias())
            continue;
         if (name.compare(alias->name, Qt::CaseInsensitive) == 0)
            return alias;
      }
      return nullptr;
   }
   Script::UserDefinedFunction* Compiler::lookup_user_defined_function(QString name) {
      auto&  list = this->functions_in_scope;
      size_t size = list.size();
      if (!size)
         return nullptr;
      for (size_t i = 0; i < size; i++) {
         auto& func = list[i];
         if (func.name.compare(name, Qt::CaseInsensitive) == 0)
            return &func;
      }
      return nullptr;
   }
   //
   /*static*/ Compiler::name_source Compiler::check_name_is_taken(const QString& name, OpcodeArgTypeRegistry::type_list_t& name_is_imported_from) {
      name_is_imported_from.clear();
      auto& type_registry = OpcodeArgTypeRegistry::get();
      //
      if (auto type = type_registry.get_variable_type(name))
         return name_source::variable_typename;
      if (auto type = type_registry.get_static_indexable_type(name))
         return name_source::static_typename;
      for (auto& member : Script::namespaces::unnamed.members)
         if (cobb::qt::stricmp(name, member.name) == 0)
            return name_source::namespace_member;
      //
      // Search the opcode lists to see if the name is in use by a non-member function:
      //
      for (auto& opcode : actionFunctionList) {
         auto& mapping = opcode.mapping;
         if (mapping.arg_context != OpcodeFuncToScriptMapping::no_context)
            //
            // We don't care about member functions.
            //
            continue;
         if (cobb::qt::stricmp(name, mapping.primary_name) == 0 || cobb::qt::stricmp(name, mapping.secondary_name) == 0) {
            switch (mapping.type) {
               case OpcodeFuncToScriptMapping::mapping_type::property_get:
               case OpcodeFuncToScriptMapping::mapping_type::property_set:
                  //
                  // Accessors are members; we don't care about them.
                  //
                  break;
               default:
                  return name_source::action;
            }
         }
      }
      for (auto& opcode : conditionFunctionList) {
         auto& mapping = opcode.mapping;
         if (mapping.arg_context != OpcodeFuncToScriptMapping::no_context)
            continue;
         if (cobb::qt::stricmp(name, mapping.primary_name) == 0 || cobb::qt::stricmp(name, mapping.secondary_name) == 0)
            return name_source::condition;
      }
      //
      // Do not allow the shadowing of imported names:
      //
      type_registry.lookup_imported_name(name, name_is_imported_from);
      if (name_is_imported_from.size())
         return name_source::imported_name;
      //
      return name_source::none;
   }
   //
   void Compiler::throw_error(const QString& text) {
      this->throw_error(this->state, text);
   }
   void Compiler::throw_error(const Script::string_scanner::pos& pos, const QString& text) {
      throw compile_exception(QString("Error on or near line %1 col %2: %e").arg(pos.line + 1).arg(pos.offset - pos.last_newline + 1).arg(text));
   }
   void Compiler::raise_error(const QString& text) {
      this->errors.emplace_back(text, this->state);
   }
   void Compiler::raise_error(const Compiler::pos& pos, const QString& text) {
      this->errors.emplace_back(text, pos);
   }
   void Compiler::raise_fatal(const QString& text) {
      this->fatal_errors.emplace_back(text, this->state);
   }
   void Compiler::raise_fatal(const Compiler::pos& pos, const QString& text) {
      this->fatal_errors.emplace_back(text, pos);
   }
   void Compiler::raise_warning(const QString& text) {
      this->warnings.emplace_back(text, this->state);
   }
   void Compiler::reset_token() {
      this->token = Token();
   }
   //
   void Compiler::parse(QString text) {
      this->text = text;
      if (!this->root) {
         this->root = new Script::Block;
         this->root->set_start(this->backup_stream_state());
      }
      this->block = this->root;
      //
      this->assignment = nullptr;
      this->comparison = nullptr;
      this->reset_token();
      this->scan([this](QChar c) {
         if (!this->is_in_statement())
            //
            // If we're not in a statement, then the next token must be a word. If that word is a 
            // keyword, then we handle it accordingly. If it is not a keyword, then it must be 
            // followed either by an operator (in which case we're in an assign statement) or by 
            // an opening parentheses (in which case the statement is a function call).
            //
            this->_parseActionStart(c);
         else if (this->assignment)
            this->_parseAssignment(c);
         //
         if (this->has_fatal())
            return true;
         return false;
      });
      if (!this->has_fatal()) {
         if (this->assignment)
            this->raise_fatal("An assignment statement is missing its righthand side.");
         if (this->comparison)
            this->raise_fatal("The file ended before a statement could be fully processed.");
         if (this->block != this->root)
            this->raise_fatal("Unclosed block.");
         if (this->next_event != Script::Block::Event::none)
            this->raise_fatal("The file ended with an \"on\" keyword but no following block.");
         this->root->set_end(this->state);
         //
         if (!this->has_errors())
            this->results.success = true;
      }
      return;
   }
   void Compiler::_parseActionStart(QChar c) {
      if (this->token.text.isEmpty()) {
         if (c != '-' && string_scanner::is_operator_char(c)) { // minus-as-numeric-sign must be special-cased
            this->raise_fatal(QString("Unexpected %1. Statements cannot begin with an operator.").arg(c));
            return;
         }
         if (string_scanner::is_syntax_char(c)) {
            this->raise_fatal(QString("Unexpected %1.").arg(c));
            return;
         }
         if (string_scanner::is_quote_char(c)) {
            this->raise_fatal(QString("Unexpected %1. Statements cannot begin with a string literal.").arg(c));
            return;
         }
         if (c == '(' || c == ')') {
            this->raise_fatal(QString("Unexpected %1. Parentheses are only allowed as delimiters for function arguments.").arg(c));
            return;
         }
         if (string_scanner::is_whitespace_char(c))
            return;
         this->token.text += c;
         this->token.pos = this->backup_stream_state();
         return;
      }
      if (c == '[') {
         this->token.brace = true;
         this->token.text += c;
         return;
      }
      if (this->token.brace) {
         if (c == ']')
            this->token.brace = false;
         this->token.text += c;
         return;
      }
      if (string_scanner::is_whitespace_char(c)) {
         this->token.ended = true;
         //
         // Handle keywords here, if appropriate.
         //
         using _handler_t = decltype(&Compiler::_handleKeyword_Alias);
         _handler_t handler = nullptr;
         //
         auto& word = this->token.text;
         if (word == "and" || word == "or" || word == "not" || word == "then") {
            this->raise_fatal(QString("The \"%1\" keyword cannot appear here.").arg(word));
            return;
         }
         if (word == "alias")
            handler = &Compiler::_handleKeyword_Alias;
         else if (word == "declare")
            handler = &Compiler::_handleKeyword_Declare;
         else if (word == "do")
            handler = &Compiler::_handleKeyword_Do;
         else if (word == "else")
            handler = &Compiler::_handleKeyword_Else;
         else if (word == "elseif")
            handler = &Compiler::_handleKeyword_ElseIf;
         else if (word == "end")
            handler = &Compiler::_handleKeyword_End;
         else if (word == "for")
            handler = &Compiler::_handleKeyword_For;
         else if (word == "function")
            handler = &Compiler::_handleKeyword_Function;
         else if (word == "if")
            handler = &Compiler::_handleKeyword_If;
         else if (word == "on")
            handler = &Compiler::_handleKeyword_On;
         //
         if (handler) {
            auto prior = this->state;
            ((*this).*(handler))();
            this->reset_token();
            if (prior.offset < this->state.offset) {
               //
               // The handler code advanced the position to the end of the keyword's relevant 
               // content (e.g. the end of a block declaration). However, our containing loop 
               // (or rather, Compiler::scan call) will increment the position one more time, 
               // so we need to rewind by one.
               //
               --this->state.offset;
            }
         }
         //
         // If (handler) is null, then the word wasn't a keyword. Move to the next iteration 
         // of the parsing loop; we'll eventually feed the word to a new statement.
         //
         return;
      }
      if (string_scanner::is_quote_char(c)) {
         this->raise_fatal(QString("Unexpected %1. Statements of the form {word \"string\"} are not valid.").arg(c));
         return;
      }
      if (c == '(') {
         this->_parseFunctionCall(false);
         --this->state.offset; // _parseFunctionCall moved us to the end of the call, but we're being run from inside of a scan-functor -- effectively a loop -- so we're going to advance one more
         return;
      }
      if (c == ')' || c == ',') {
         this->raise_fatal(QString("Unexpected %1.").arg(c));
         return;
      }
      if (string_scanner::is_operator_char(c)) {
         this->assignment = new Script::Statement;
         this->assignment->set_start(this->token.pos);
         {
            this->assignment->lhs = new Script::VariableReference(this->token.text);
            auto ref = this->assignment->lhs;
            ref->owner = this->assignment;
            ref->resolve(*this);
            if (ref->is_constant_integer())
               this->raise_error("Cannot assign to a constant integer.");
            else if (ref->is_read_only())
               this->raise_error(QString("Cannot assign to \"%1\". The referenced value is read-only.").arg(ref->to_string()));
         }
         this->reset_token();
         this->token.text = c;
         this->token.pos  = this->backup_stream_state();
         return;
      }
      if (this->token.ended) {
         this->raise_fatal("Statements of the form {word word} are not valid.");
         return;
      }
      this->token.text += c;
      if (this->token.text[0] == '-' && !c.isNumber()) {
         //
         // We allowed the word to start with "-" in case it was a number, but it 
         // has turned out not to be a number. That means that the "-" was an 
         // operator, not a numeric sign. Wait, that's illegal.
         //
         this->raise_fatal("Unexpected -. Statements cannot begin with an operator.");
         return;
      }
   }
   void Compiler::_parseAssignment(QChar c) {
      assert(this->assignment && "This should not have been called!");
      #pragma region Code to finish parsing an assignment (i.e. the operator and righthand side)
         if (this->assignment->op.isEmpty()) {
            //
            // If the statement doesn't have an operator stored, then the operator is currently 
            // being parsed and exists in (token).
            //
            if (string_scanner::is_operator_char(c)) {
               this->token.text += c;
               return;
            }
            auto a = this->assignment;
            a->op = this->token.text;
            if (!_is_assignment_operator(this->token.text))
               this->raise_error(QString("Operator %1 is not a valid assignment operator.").arg(this->token.text));
            this->reset_token();
            //
            // Fall through to righthand-side handling so we don't miss the first character 
            // after the operator in cases like {a=b} where there's no whitespace.
            //
         }
         if ((!this->token.text.isEmpty() || c != '-') && string_scanner::is_operator_char(c)) {
            this->raise_fatal(QString("Unexpected %1 on the righthand side of an assignment statement.").arg(c));
            return;
         }
         if (string_scanner::is_quote_char(c)) {
            this->raise_fatal(QString("Unexpected %1. You cannot assign strings to variables.").arg(c));
            return;
         }
         if (this->token.text.isEmpty()) {
            if (string_scanner::is_whitespace_char(c))
               return;
            if (c == '(') {
               this->raise_fatal("Unexpected (. Parentheses are only allowed as delimiters for function arguments.");
               return;
            }
         } else {
            if (c == '(') {
               if (this->assignment->op != ce_assignment_operator) {
                  this->raise_error(QString("Operator %1 cannot be used to assign the result of a function call to a variable. Use operator =.").arg(this->assignment->op));
               }
               this->_parseFunctionCall(false);
               --this->state.offset; // _parseFunctionCall moved us to the end of the call, but we're being run from inside of a scan-functor -- effectively a loop -- so we're going to advance one more
               return;
               //
               // From here on out, the code for parsing function calls will handle what 
               // remains. The end of the function call is also the end of this statement, 
               // so the code for parsing function calls will "close" this statement just 
               // fine.
               //
            }
         }
         if (c == ')' || c == ',') {
            this->raise_fatal(QString("Unexpected %1.").arg(c));
            return;
         }
         if (!string_scanner::is_whitespace_char(c)) {
            if (this->token.text.isEmpty())
               this->token.pos = this->backup_stream_state();
            this->token.text += c;
            return;
         }
      #pragma endregion
      //
      // If we get here, then we've encountered the end of the statement's righthand side.
      //
      #pragma region Code to compile an assignment
         this->assignment->set_end(this->state);
         this->assignment->rhs = new Script::VariableReference(this->token.text);
         this->assignment->rhs->owner = this->assignment;
         //
         // TODO: Consider writing code for type checking non-accessor assignments in advance... but 
         // maybe leave it turned off: I want to check the results of type-mismatched assignments in-
         // game. I'm pretty sure that numbers and timers can be assigned to each other, but I want to 
         // know if other types do anything sensible, do nothing at all, clear the target variable, or 
         // crash, so that I can program in a compiler warning or compiler error as appropriate.
         //

         //
         // Compile the assignment opcode:
         //
         auto opcode = std::make_unique<Action>();
         {
            auto lhs = this->assignment->lhs;
            auto rhs = this->assignment->rhs;
            auto l_accessor = lhs->get_accessor_definition();
            auto r_accessor = rhs->get_accessor_definition();
            if (l_accessor || r_accessor) {
               //
               // This is an accessor assignment, not a standard assignment.
               //
               if (l_accessor && r_accessor)
                  this->throw_error("Cannot assign one accessor to another accessor.");
               const OpcodeFuncToScriptMapping* mapping = nullptr;
               const OpcodeBase* accessor = nullptr;
               QString acc_name;
               if (l_accessor) {
                  auto setter = l_accessor->setter;
                  if (!setter) {
                     this->raise_error("This accessor cannot be assigned to.");
                     return;
                  }
                  accessor = setter;
                  acc_name = lhs->resolved.accessor_name;
                  mapping  = &setter->mapping;
                  lhs->strip_accessor();
                  //
                  // Compile the left-hand side (the context-argument):
                  //
                  auto ai  = mapping->arg_context;
                  auto arg = (accessor->arguments[ai].typeinfo.factory)();
                  opcode->arguments[ai] = arg;
                  arg->compile(*this, *lhs, 0);
                  //
                  // Compile the right-hand side (the value to assign):
                  //
                  ai  = accessor->index_of_operand_argument();
                  arg = (accessor->arguments[ai].typeinfo.factory)();
                  opcode->arguments[ai] = arg;
                  arg->compile(*this, *rhs, 0);
               } else {
                  auto getter = r_accessor->getter;
                  if (!getter) {
                     this->raise_error("This accessor cannot be read.");
                     return;
                  }
                  accessor = getter;
                  acc_name = rhs->resolved.accessor_name;
                  mapping  = &getter->mapping;
                  rhs->strip_accessor();
                  //
                  // Compile the left-hand side (the out-argument):
                  //
                  int  ai  = accessor->index_of_out_argument();
                  auto arg = (accessor->arguments[ai].typeinfo.factory)();
                  opcode->arguments[ai] = arg;
                  arg->compile(*this, *lhs, 0);
                  //
                  // Compile the right-hand side (the value to assign):
                  //
                  ai  = mapping->arg_context;
                  arg = (accessor->arguments[ai].typeinfo.factory)();
                  opcode->arguments[ai] = arg;
                  arg->compile(*this, *rhs, 0);
               }
               assert(mapping);
               if (accessor->get_name_type()) {
                  //
                  // The accessor is variably named. We need to compile the name.
                  //
                  auto  op_string = string_scanner(acc_name);
                  auto  ai   = mapping->arg_name;
                  auto& base = accessor->arguments[ai];
                  opcode->arguments[ai] = (base.typeinfo.factory)();
                  opcode->arguments[ai]->compile(*this, op_string, 0);
               }
               if (mapping->arg_operator == OpcodeFuncToScriptMapping::no_argument) {
                  //
                  // This accessor doesn't have an "operator" argument, so throw an error if we're using the 
                  // wrong argument. (We should only be using accessors in the first place if there *is* an 
                  // opcode argument, but it's possible that only one of the getter and setter may have it.)
                  //
                  if (this->assignment->op != "=")
                     this->raise_error("This accessor can only be invoked using the = operator.");
               } else {
                  //
                  // Compile the assignment operator.
                  //
                  auto op_string = string_scanner(this->assignment->op);
                  auto op_arg    = (accessor->arguments[mapping->arg_operator].typeinfo.factory)();
                  opcode->arguments[mapping->arg_operator] = op_arg;
                  op_arg->compile(*this, op_string, 0);
               }
            } else {
               auto base = &_get_assignment_opcode();
               opcode->function = base;
               opcode->arguments.resize(3);
               opcode->arguments[0] = (base->arguments[0].typeinfo.factory)();
               opcode->arguments[1] = (base->arguments[1].typeinfo.factory)();
               opcode->arguments[2] = (base->arguments[2].typeinfo.factory)();
               opcode->arguments[0]->compile(*this, *lhs, 0);
               opcode->arguments[1]->compile(*this, *rhs, 0);
               //
               auto op_string = string_scanner(this->assignment->op);
               opcode->arguments[2]->compile(*this, op_string, 0);
            }
         }
         this->assignment->opcode = opcode.release();
         //
         this->block->insert_item(this->assignment);
         this->assignment = nullptr;
         this->reset_token();
      #pragma endregion
   }
   //
   void Compiler::_applyConditionModifiers(Script::Comparison* condition) {
      if (!condition)
         return;
      auto opcode = dynamic_cast<Condition*>(condition->opcode);
      if (opcode) // it could have failed to compile
         opcode->inverted = this->negate_next_condition;
      if (this->next_condition_joiner == c_joiner::or)
         condition->next_is_or = true;
      this->next_condition_joiner = c_joiner::none;
      this->negate_next_condition = false;
   }
   //
   void Compiler::_parseBlockConditions() {
      this->comparison = nullptr;
      this->reset_token();
      this->scan([this](QChar c) {
         if (!this->comparison) {
            if (this->_parseConditionStart(c))
               return true; // stop the loop; we found the "then" keyword
            if (this->has_fatal())
               return true;
            return false;
         }
         //
         // If, on the other hand, we're in a statement, then we need to finish that up.
         //
         this->_parseComparison(c);
         if (this->has_fatal())
            return true;
         return false;
      });
   }
   bool Compiler::_parseConditionStart(QChar c) {
      if (this->token.text.isEmpty()) {
         if (c != '-' && string_scanner::is_operator_char(c)) { // minus-as-numeric-sign must be special-cased
            this->raise_fatal(QString("Unexpected %1. Conditions cannot begin with an operator.").arg(c));
            return;
         }
         if (string_scanner::is_syntax_char(c)) {
            this->raise_fatal(QString("Unexpected %1.").arg(c));
            return;
         }
         if (string_scanner::is_quote_char(c)) {
            this->raise_fatal(QString("Unexpected %1. Conditions cannot begin with a string literal.").arg(c));
            return;
         }
         if (c == '(' || c == ')') {
            this->raise_fatal(QString("Unexpected %1. Parentheses are only allowed as delimiters for function arguments.").arg(c));
            return;
         }
         if (string_scanner::is_whitespace_char(c))
            return false;
         this->token.text += c;
         this->token.pos = this->backup_stream_state();
         return false;
      }
      if (c == '[') {
         this->token.brace = true;
         this->token.text += c;
         return false;
      }
      if (this->token.brace) {
         if (c == ']')
            this->token.brace = false;
         this->token.text += c;
         return false;
      }
      if (string_scanner::is_whitespace_char(c)) {
         this->token.ended = true;
         //
         // Handle keywords here, if appropriate.
         //
         auto& word = this->token.text;
         if (word == "then") {
            if (this->negate_next_condition)
               this->raise_fatal("Expected a condition after \"not\".");
            switch (this->next_condition_joiner) {
               case c_joiner::and:
                  this->raise_fatal("Expected a condition after \"and\".");
                  break;
               case c_joiner::or:
                  this->raise_fatal("Expected a condition after \"or\".");
                  break;
            }
            return true;
         }
         if (word == "alias")
            this->raise_fatal(QString("You cannot place %1 declarations inside of conditions.").arg(word));
         else if (word == "do")
            this->raise_fatal("You cannot open or close blocks inside of conditions. (If the \"do\" was meant to mark the end of conditions, use \"then\" instead.)");
         else if (word == "else" || word == "elseif")
            this->raise_fatal(QString("Unexpected \"%1\". A list of conditions must end with \"then\".").arg(word));
         else if (word == "for" || word == "function" || word == "if")
            this->raise_fatal("You cannot open or close blocks inside of conditions. End the list of conditions using the \"then\" keyword.");
         else if (word == "on")
            this->raise_fatal("You cannot mark event handlers inside of conditions.");
         else if (word == "and") {
            if (this->negate_next_condition) // this check only works because we do not allow (not not condition)
               this->raise_fatal("Constructions of the form {not and} and {not or} are not valid.");
            else if (this->next_condition_joiner != c_joiner::none)
               this->raise_fatal("Constructions of the form {or and} and {and and} are not valid.");
            else
               this->next_condition_joiner = c_joiner::and;
            //
            this->reset_token();
         } else if (word == "or") {
            if (this->negate_next_condition) // this check only works because we do not allow (not not condition)
               this->raise_fatal("Constructions of the form {not and} and {not or} are not valid.");
            else if (this->next_condition_joiner != c_joiner::none)
               this->raise_fatal("Constructions of the form {and or} and {or or} are not valid.");
            else
               this->next_condition_joiner = c_joiner::or;
            //
            this->reset_token();
         } else if (word == "not") {
            if (this->negate_next_condition) {
               //
               // NOTE: If we decide to allow (not not condition) and just have each "not" toggle the negate-next-condition flag, 
               // then we also need to modify the code that checks for (not and condition) and (not or condition) in order to make 
               // sure that (not not and condition) and (not not or condition) are still considered invalid. That code is in this 
               // same function.
               //
               this->raise_fatal("Constructions of the form {not not condition} are not valid. Use a single \"not\" or no \"not\" at all.");
               return;
            }
            this->negate_next_condition = true;
            this->reset_token();
         }
         return false;
      }
      if (string_scanner::is_quote_char(c)) {
         this->raise_fatal(QString("Unexpected %1. Statements of the form {word \"string\"} are not valid.").arg(c));
         return;
      }
      if (c == '(') {
         this->_parseFunctionCall(true);
         --this->state.offset; // _parseFunctionCall moved us to the end of the call, but we're being run from inside of a scan-functor -- effectively a loop -- so we're going to advance one more
         return false;
      }
      if (c == ')' || c == ',') {
         this->raise_fatal(QString("Unexpected %1.").arg(c));
      }
      if (string_scanner::is_operator_char(c)) {
         this->comparison = new Script::Comparison;
         this->comparison->set_start(this->token.pos);
         this->comparison->lhs = new Script::VariableReference(this->token.text);
         this->comparison->lhs->owner = this->assignment;
         this->reset_token();
         this->token.text = c;
         this->token.pos = this->backup_stream_state();
         return false;
      }
      if (this->token.ended) {
         this->raise_fatal("Statements of the form {word word} are not valid.");
      }
      this->token.text += c;
      if (this->token.text[0] == '-' && !c.isNumber()) {
         //
         // We allowed the word to start with "-" in case it was a number, but it 
         // has turned out not to be a number. That means that the "-" was an 
         // operator, not a numeric sign. Wait, that's illegal.
         //
         this->raise_fatal("Unexpected -. Statements cannot begin with an operator.");
         return;
      }
      return false;
   }
   void Compiler::_parseComparison(QChar c) {
      assert(this->comparison && "This should not have been called!");
      #pragma region Code to finish parsing a comparison (i.e. the operator and righthand side)
         if (this->comparison->op.isEmpty()) {
            //
            // If the statement doesn't have an operator stored, then the operator is currently 
            // being parsed and exists in (token).
            //
            if (string_scanner::is_operator_char(c)) {
               this->token.text += c;
               return;
            }
            auto c = this->comparison;
            c->op = this->token.text;
            if (!_is_comparison_operator(this->token.text))
               this->raise_error(QString("Operator %1 is not a valid comparison operator.").arg(this->token.text));
            this->reset_token();
            //
            // Fall through to righthand-side handling so we don't miss the first character 
            // after the operator in cases like {a==b} where there's no whitespace.
            //
         }
         //
         // Handle the righthand side.
         //
         if ((!this->token.text.isEmpty() || c != '-') && string_scanner::is_operator_char(c)) {
            this->raise_fatal(QString("Unexpected %1 on the righthand side of a comparison statement.").arg(c));
            return;
         }
         if (string_scanner::is_quote_char(c)) {
            this->raise_error(QString("Unexpected %1. You cannot compare variables to strings.").arg(c));
            this->skip_to(c); // TODO: double-check this; I'm not sure it'll work properly
            return;
         }
         if (this->token.text.isEmpty() && string_scanner::is_whitespace_char(c))
            return;
         if (c == "(") {
            if (!this->token.text.isEmpty())
               this->raise_fatal(QString("Unexpected %1. You cannot compare variables to the result of a function call.").arg(c));
            this->raise_fatal(QString("Unexpected %1. Parentheses are only allowed as delimiters for function arguments.").arg(c));
            return;
         }
         if (c == ')' || c == ',') {
            this->raise_fatal(QString("Unexpected %1.").arg(c));
            return;
         }
         if (!string_scanner::is_whitespace_char(c)) {
            if (this->token.text.isEmpty())
               this->token.pos = this->backup_stream_state();
            this->token.text += c;
            return;
         }
      #pragma endregion
      //
      // If we get here, then we've encountered the end of the statement's righthand side.
      //
      #pragma region Code to compile a comparison
         this->comparison->set_end(this->state);
         this->comparison->rhs = new Script::VariableReference(this->token.text);
         this->comparison->rhs->owner = this->comparison;
         //
         {
            auto lhs  = this->assignment->lhs;
            auto rhs  = this->assignment->rhs;
            bool fail = false;
            if (lhs->get_accessor_definition()) {
               this->raise_error(QString("Cannot use accessors such as \"%1\" in comparisons.").arg(lhs->to_string()));
               fail = true;
            }
            if (rhs->get_accessor_definition()) {
               this->raise_error(QString("Cannot use accessors such as \"%1\" in comparisons.").arg(rhs->to_string()));
               fail = true;
            }
            if (!fail) {
               auto opcode = std::make_unique<Condition>();
               //
               auto base = &_get_comparison_opcode();
               opcode->function = base;
               opcode->arguments.resize(3);
               opcode->arguments[0] = (base->arguments[0].typeinfo.factory)();
               opcode->arguments[1] = (base->arguments[1].typeinfo.factory)();
               opcode->arguments[2] = (base->arguments[2].typeinfo.factory)();
               opcode->arguments[0]->compile(*this, *lhs, 0);
               opcode->arguments[1]->compile(*this, *rhs, 0);
               //
               auto op_string = string_scanner(this->assignment->op);
               opcode->arguments[2]->compile(*this, op_string, 0);
               //
               this->comparison->opcode = opcode.release();
            }
         }
         this->_applyConditionModifiers(this->comparison);
         //
         this->block->insert_condition(this->comparison);
         this->comparison = nullptr;
         this->reset_token();
      #pragma endregion
   }
   //
   void Compiler::__parseFunctionArgs(const OpcodeBase& function, Opcode& opcode) {
      auto& mapping = function.mapping;
      opcode.arguments.resize(function.arguments.size());
      //
      int8_t opcode_arg_index = 0;
      int8_t opcode_arg_part  = 0;
      int8_t script_arg_index = 0;
      std::unique_ptr<OpcodeArgValue> current_argument = nullptr;
      bool comma = false;
      do {
         if (opcode_arg_index >= mapping.mapped_arg_count()) {
            this->throw_error("Too many arguments passed to the function.");
         }
         QString raw_argument;
         {
            comma = false;
            //
            QChar delim = '\0';
            this->scan([this, &comma, &delim, &raw_argument](QChar c) {
               if (delim == '\0') { // can't use a constexpr for the "none" value because lambdas don't like that, and can't use !delim because a null QChar doesn't test as false, UGH
                  if (c == ',' || c == ')') {
                     comma = (c == ',');
                     return true; // stop
                  }
                  if (c == '[')
                     delim = ']';
                  else if (string_scanner::is_quote_char(c))
                     delim = c;
               } else {
                  if (c == delim)
                     delim = '\0';
               }
               raw_argument += c;
               return false;
            });
         }
         //
         auto& base = function.arguments[mapping.arg_index_mappings[script_arg_index]];
         script_arg_index++;
         if (!current_argument) {
            current_argument.reset((base.typeinfo.factory)());
            if (!current_argument) {
               this->throw_error("Unknown error: failed to instantiate an OpcodeArgValue while parsing arguments to the function call.");
            }
         }
         //
         string_scanner argument(raw_argument);
         arg_compile_result result = current_argument->compile(*this, argument, opcode_arg_part);
         switch (result) {
            case arg_compile_result::success:
               opcode.arguments[opcode_arg_index] = current_argument.release();
               ++opcode_arg_index;
               opcode_arg_part = 0;
               break;
            case arg_compile_result::failure:
               this->throw_error(QString("Failed to parse script argument %1.").arg(script_arg_index - 1)); // TODO: this won't allow us to handle overloads
            case arg_compile_result::needs_another:
               ++opcode_arg_part;
               if (!comma) {
                  this->throw_error("Not enough arguments passed to the function.");
               }
               break;
            case arg_compile_result::can_take_another:
               ++opcode_arg_part;
         }
         this->state += argument.backup_stream_state();
      } while (comma);
      if (opcode_arg_index < mapping.mapped_arg_count())
         this->throw_error("Not enough arguments passed to the function.");
   }
   namespace {
      template<typename T, int I> void _find_opcode_bases(const std::array<T, I>& list, std::vector<const OpcodeBase*>& results, QString function_name, Script::VariableReference* context) {
         for (auto& action : list) {
            auto& mapping = action.mapping;
            if (context) {
               if (mapping.arg_context == OpcodeFuncToScriptMapping::no_context)
                  continue;
               auto& base = action.arguments[mapping.arg_context];
               if (&base.typeinfo != context->get_type())
                  continue;
            } else {
               if (mapping.arg_context != OpcodeFuncToScriptMapping::no_context)
                  continue;
            }
            if (cobb::qt::stricmp(function_name, mapping.primary_name) == 0 || cobb::qt::stricmp(function_name, mapping.secondary_name) == 0)
               results.push_back(&action);
         }
      }
   }
   void Compiler::_parseFunctionCall(bool is_condition) {
      //
      // When this function is called, the stream position should be just after the 
      // opening parentheses for the call arguments. Assuming no syntax errors are 
      // encountered, this function advances the stream position to just after the 
      // ")" glyph that marks the end of the call arguments; if you are calling 
      // this function from inside a functor being run by MSimpleParser.scan, then 
      // you will need to rewind the stream position by one to avoid skipping the 
      // glyph after the ")", because the functor is being run in a loop and the 
      // loop will advance the stream by one more character.
      //
      // At the time this function is called, (this->token) should refer to the 
      // combined context (if any) and function name.
      //
      // Called from _parseActionStart, _parseConditionStart, and _parseAssignment.
      //
      auto call_start = this->token.pos;
      QString function_name;
      std::unique_ptr<Script::VariableReference> context = nullptr;
      {  // Identify the context and the function name, i.e. context.function_name(arg, arg, arg)
         auto& text = this->token.text;
         //
         int size = text.size();
         int i    = size - 1;
         for (; i >= 0; --i) {
            auto c = text[i];
            if (QString("[]").indexOf(c) >= 0) {
               this->raise_fatal(call_start, "Function names cannot contain square brackets.");
               return;
            }
            if (c == '.') {
               function_name = text.chopped(i + 1); // oh, but be sure not to confuse this with (QString::chop), which does the literal exact opposite!
               break;
            }
         }
         if (function_name.isEmpty()) { // there was no '.', or it was at the end
            if (i == size - 1) { // "name.()"
               this->raise_fatal(call_start, "Constructions of the form {name.()} are syntax errors. A function name is required.");
               return;
            }
            function_name = text;
         } else {
            context.reset(new Script::VariableReference(text.mid(0, i)));
            context->resolve(*this);
         }
      }
      this->reset_token();
      ++this->state.offset; // advance past the open-paren
      //
      if (!context) {  // Handle user-defined function calls
         auto func = this->lookup_user_defined_function(function_name);
         if (func) {
            if (is_condition) {
               this->raise_error(call_start, QString("User-defined functions such as \"%1\" cannot be called from inside of a condition.").arg(function_name));
               this->skip_to(')');
               return;
            }
            if (this->assignment) {
               this->raise_error(call_start, QString("User-defined functions such as \"%1\" cannot return values.").arg(function_name));
               this->skip_to(')');
               return;
            }
            if (!this->extract_specific_char(')')) {
               this->raise_error(call_start, QString("Expected ')'. User-defined functions such as \"%1\" cannot have arguments passed to them.").arg(function_name));
               this->skip_to(')');
               return;
            }
            //
            // Now, we need to compile a call to the user-defined function.
            //
            // Normally, we only create a Block's Trigger when we're compiling the block in full. 
            // However, in order to compile calls to a user-defined function as we find them, we 
            // need to be able to know the index of that function's final trigger... which means 
            // that the function needs to *have* a trigger. So, we create the trigger at the 
            // time that we open the function.
            //
            assert(func->trigger_index >= 0 && "A user-defined function, tracked by the compiler, has not had its trigger index stored properly.");
            //
            auto  opcode = new Action;
            auto& base   = actionFunction_runNestedTrigger;
            auto  arg    = (base.arguments[0].typeinfo.factory)();
            opcode->function = &base;
            opcode->arguments.push_back(arg);
            auto arg_c   = dynamic_cast<OpcodeArgValueTrigger*>(arg);
            assert(arg_c && "The argument to the ''run nested trigger'' opcode isn't OpcodeArgValueTrigger anymore? Did someone change the opcode-base?");
            arg_c->value = func->trigger_index;
            //
            auto statement = new Script::Statement;
            statement->opcode = opcode;
            statement->set_start(call_start);
            statement->set_end(this->state);
            this->block->insert_item(statement);
            return;
         }
      }
      //
      std::vector<const OpcodeBase*> opcode_bases;
      if (is_condition) {
         _find_opcode_bases(conditionFunctionList, opcode_bases, function_name, context.get());
      } else {
         _find_opcode_bases(actionFunctionList, opcode_bases, function_name, context.get());
      }
      if (!opcode_bases.size()) {
         if (context)
            this->raise_error(call_start, QString("Type %1 does not have a member function named \"%2\".").arg(context->get_type()->internal_name.c_str()).arg(function_name));
         else
            this->raise_error(call_start, QString("There is no non-member function named \"%1\".").arg(function_name));
         this->skip_to(')');
         return;
      }
      //
      const OpcodeBase* match = nullptr;
      auto start = this->backup_stream_state();
      std::unique_ptr<Opcode> opcode;
      if (is_condition) {
         opcode.reset(new Condition);
      } else {
         opcode.reset(new Action);
      }
      for (auto* function : opcode_bases) {
         //
         // If two opcodes have the same name and context (or lack thereof), then they are overloads of 
         // each other with different arguments. Use trial-and-error to determine which one the script 
         // author is invoking.
         //
         opcode->reset();
         this->restore_stream_state(start);
         try {
            this->__parseFunctionArgs(*function, *opcode.get());
            match = function;
            break;
         } catch (compile_exception& e) {
            if (opcode_bases.size() == 1)
               //
               // If we aren't dealing with function overloads, then just use the original parse error.
               //
               throw e;
         }
      }
      if (!match)
         //
         // We'll get here if we're dealing with function overloads and none of the overloads matched.
         //
         this->throw_error(call_start, QString("The arguments you passed to %1.%2 did not match any of its function signatures.").arg(context->get_type()->internal_name.c_str()).arg(function_name));
      //
      // If we've reached this point without any errors, then we should be just before the terminating ')' for the function call.
      //
      if (this->assignment) {
         //
         // We're assigning the return value of this function call to something, so let's first make 
         // sure that the function actually returns a value.
         //
         auto index = match->index_of_out_argument();
         bool fail  = false;
         if (index < 0) {
            this->raise_error(call_start, QString("Function %1.%2 does not return a value.").arg(context->get_type()->internal_name.c_str()).arg(function_name));
            fail = true;
         }
         if (this->assignment->lhs->is_read_only()) {
            this->raise_error("You cannot assign to this value.");
            fail = true;
         } else if (this->assignment->lhs->is_accessor()) {
            this->raise_error("You cannot assign the return value of a function to an accessor.");
            fail = true;
         }
         const OpcodeArgBase& base = match->arguments[index];
         //
         // Verify that the variable we're assigning our return value to is of the right type:
         //
         auto target_type = this->assignment->lhs->get_type();
         if (&base.typeinfo != target_type) {
            this->raise_error(call_start, QString("Function %1.%2 returns a %3, not a %4.")
               .arg(context->get_type()->internal_name.c_str())
               .arg(function_name)
               .arg(base.typeinfo.internal_name.c_str())
               .arg(target_type->internal_name.c_str())
            );
            fail = true;
         }
         //
         if (!fail) {
            //
            // The type is correct, so set the out-argument.
            //
            opcode->arguments[index] = (base.typeinfo.factory)();
            opcode->arguments[index]->compile(*this, *this->assignment->lhs, 0);
         }
      }
      if (!this->extract_specific_char(')')) {
         this->raise_fatal("Expected ')'.");
         return;
      }
      this->reset_token();
      Script::Statement* statement = this->assignment;
      if (!statement) {
         if (is_condition)
            statement = new Script::Comparison;
         else
            statement = new Script::Statement;
      }
      statement->opcode = opcode.release();
      statement->set_end(this->state);
      if (is_condition) {
         this->_applyConditionModifiers(dynamic_cast<Script::Comparison*>(statement));
         this->block->insert_condition(statement);
      } else {
         this->block->insert_item(statement);
      }
      this->assignment = nullptr;
   }
   //
   bool Compiler::is_in_statement() const {
      return this->assignment || this->comparison;
   }
   int32_t Compiler::_index_of_trigger(Trigger* t) const noexcept {
      auto&  list = this->results.triggers;
      size_t size = list.size();
      for (size_t i = 0; i < size; ++i)
         if (list[i] == t)
            return i;
      return -1;
   }
   //
   void Compiler::_openBlock(Script::Block* block) {
      if (this->block == this->root) {
         //
         // We're opening a top-level Block. Before we do that, let's see if the root block contains any 
         // statements; if so, let's put those in their own trigger.
         //
         assert(this->root->trigger == nullptr);
         this->root->compile(*this);
         this->root->trigger = nullptr;
         this->root->clear();
      }
      this->block = block;
   }
   bool Compiler::_closeCurrentBlock() {
      if (this->block == this->root)
         return false;
      this->block->set_end(this->state);
      {  // The block's aliases are going out of scope.
         size_t size = this->aliases_in_scope.size();
         if (size > 0) {
            signed int i = size - 1;
            for (; i >= 0; --i) {
               auto alias = this->aliases_in_scope[i];
               if (alias->parent != this->block)
                  break;
            }
            ++i; // convert from "index of last alias to keep" to "number of aliases to keep"
            if (i < size)
               this->aliases_in_scope.resize(i);
         }
      }
      {  // The block's contained functions are going out of scope.
         auto& list = this->functions_in_scope;
         list.erase(std::remove_if(list.begin(), list.end(),
            [this](Script::UserDefinedFunction& entry) {
               auto block = entry.block;
               if (!block || block->parent == this->block)
                  return true;
               return false;
            }),
            list.end()
         );
      }
      auto parent = dynamic_cast<Script::Block*>(this->block->parent);
      if (!parent)
         return false;
      if (parent == this->root) {
         this->block->compile(*this);
      }
      this->block = parent;
      return true;
   }
   //
   void Compiler::_handleKeyword_Alias() {
      auto start = this->token.pos;
      //
      auto name = this->extract_word();
      if (name.isEmpty()) {
         this->raise_fatal("An alias declaration must supply a name.");
         return;
      }
      if (!this->extract_specific_char('=')) {
         this->raise_fatal("Expected \"=\".");
         return;
      }
      auto target = this->extract_word();
      if (target.isEmpty()) {
         this->raise_fatal("An alias declaration must supply a target.");
         return;
      }
      //
      auto item = new Script::Alias(*this, name, target);
      item->set_start(start);
      item->set_end(this->state);
      this->block->insert_item(item);
      this->aliases_in_scope.push_back(item);
   }
   void Compiler::_handleKeyword_Declare() {
      auto start = this->token.pos;
      //
      auto name = this->extract_word();
      if (name.isEmpty()) {
         this->raise_fatal("The \"declare\" statement must be used to declare a variable.");
         return;
      }
      //
      // TODO: process the name
      //
      if (!this->extract_specific_char('='))
         return;
      int32_t initial = 0;
      if (this->extract_integer_literal(initial)) {
         //
         // TODO: process the initial value
         //
      } else {
         auto word = this->extract_word();
         //
         // TODO: process the initial value
         //
      }
   }
   void Compiler::_handleKeyword_Do() {
      auto item = new Script::Block;
      item->type = Script::Block::Type::basic;
      item->set_start(this->token.pos);
      item->event = this->next_event;
      this->next_event = Script::Block::Event::none;
      this->block->insert_item(item);
      this->_openBlock(item);
   }
   void Compiler::_handleKeyword_Else() {
      if (this->block->type != Script::Block::Type::if_block && this->block->type != Script::Block::Type::elseif_block) {
         auto prev = this->block->item(-1);
         auto p_bl = dynamic_cast<Script::Block*>(prev);
         if (p_bl) {
            if (p_bl->type == Script::Block::Type::if_block || p_bl->type == Script::Block::Type::elseif_block)
               this->raise_fatal("Unexpected \"else\". This keyword should not be preceded by the \"end\" keyword.");
         }
         this->raise_fatal("Unexpected \"else\".");
         return;
      }
      if (!this->_closeCurrentBlock()) {
         this->raise_fatal("Unexpected \"else\".");
         return;
      }
      auto item = new Script::Block;
      item->type = Script::Block::Type::else_block;
      item->set_start(this->token.pos);
      this->block->insert_item(item);
      this->_openBlock(item);
   }
   void Compiler::_handleKeyword_ElseIf() {
      if (this->block->type != Script::Block::Type::if_block && this->block->type != Script::Block::Type::elseif_block) {
         auto prev = this->block->item(-1);
         auto p_bl = dynamic_cast<Script::Block*>(prev);
         if (p_bl) {
            if (p_bl->type == Script::Block::Type::if_block || p_bl->type == Script::Block::Type::elseif_block)
               this->raise_fatal("Unexpected \"elseif\". This keyword should not be preceded by the \"end\" keyword.");
         }
         this->raise_fatal("Unexpected \"elseif\".");
         return;
      }
      if (!this->_closeCurrentBlock()) {
         this->raise_fatal("Unexpected \"elseif\".");
         return;
      }
      auto item = new Script::Block;
      item->type = Script::Block::Type::elseif_block;
      item->set_start(this->token.pos);
      this->block->insert_item(item);
      this->_openBlock(item);
      this->_parseBlockConditions();
      this->reset_token();
   }
   void Compiler::_handleKeyword_End() {
      if (!this->_closeCurrentBlock())
         this->raise_fatal("Unexpected \"end\".");
   }
   void Compiler::_handleKeyword_If() {
      auto item = new Script::Block;
      item->type = Script::Block::Type::if_block;
      item->set_start(this->token.pos);
      item->event = this->next_event;
      this->next_event = Script::Block::Event::none;
      this->block->insert_item(item);
      this->_openBlock(item);
      this->_parseBlockConditions();
      this->reset_token();
   }
   void Compiler::_handleKeyword_For() {
      auto start = this->token.pos;
      //
      if (!this->extract_word("each"))
         this->throw_error("The \"for\" keyword must be followed by \"each\".");
      auto word = this->extract_word();
      if (word.isEmpty())
         this->throw_error("Invalid for-loop.");
      auto    type = Script::Block::Type::basic;
      QString label;
      int32_t label_index = -1;
      bool    label_is_index = false;
      if (word == "team") {
         type = Script::Block::Type::for_each_team;
         if (!this->extract_word("do"))
            this->throw_error("Invalid for-each-team loop: expected the word \"do\".");
      } else if (word == "player") {
         type = Script::Block::Type::for_each_player;
         word = this->extract_word();
         if (word == "randomly") {
            type = Script::Block::Type::for_each_player_randomly;
            word = this->extract_word();
         }
         if (word != "do") {
            if (type == Script::Block::Type::for_each_player_randomly)
               this->throw_error("Invalid for-each-player-randomly loop: expected the word \"do\".");
            this->throw_error("Invalid for-each-player loop: expected the word \"randomly\" or the word \"do\".");
         }
      } else if (word == "object") {
         type = Script::Block::Type::for_each_object;
         word = this->extract_word();
         if (word != "do") {
            if (word != "with")
               this->throw_error("Invalid for-each-object loop: expected the word \"with\" or the word \"do\".");
            type = Script::Block::Type::for_each_object_with_label;
            word = this->extract_word();
            if (word == "no") {
               if (!this->extract_word("label"))
                  this->throw_error("Invalid for-each-object-with-label loop: must use the phrase \"no label\" or specify a label.");
            } else {
               if (word != "label")
                  this->throw_error("Invalid for-each-object-with-label loop: expected the word \"label\".");
               if (!this->extract_string_literal(label)) {
                  if (!this->extract_integer_literal(label_index))
                     this->throw_error("Invalid for-each-object-with-label loop: the label must be specified as a string literal or as a numeric label index.");
                  label_is_index = true;
               }
            }
            if (!this->extract_word("do"))
               this->throw_error("Invalid for-each-object-with-label loop: expected the word \"do\".");
         }
      } else {
         this->throw_error("Invalid for-loop.");
      }
      //
      auto item = new Script::Block;
      item->type        = type;
      item->label_name  = label;
      item->label_index = label_index;
      item->set_start(start);
      item->event       = this->next_event;
      this->next_event  = Script::Block::Event::none;
      this->block->insert_item(item);
      this->_openBlock(item);
   }
   void Compiler::_handleKeyword_Function() {
      auto start = this->token.pos;
      //
      auto name = this->extract_word();
      if (name.isEmpty()) {
         this->raise_fatal("A function must have a name.");
         return;
      }
      if (name[0].isNumber()) {
         //
         // Do not allow a function's name to start with a number. We want this to be consistent with 
         // alias names, which disallow numbers at their start so that it's easier for opcode argument 
         // compile functions to check for both integer literals and integer alias names.
         //
         this->throw_error("A function's name cannot begin with a number.");
         return;
      }
      for (QChar c : name) {
         if (QString("[].").contains(c)) {
            this->raise_fatal(QString("Unexpected %1 inside of a function name.").arg(c));
            return;
         }
      }
      if (Compiler::is_keyword(name)) {
         this->raise_fatal(QString("Keyword \"%1\" cannot be used as the name of a function.").arg(name));
         return;
      }
      if (!this->extract_specific_char('(')) {
         this->raise_fatal("Expected \"(\".");
         return;
      }
      if (!this->extract_specific_char(')')) {
         this->raise_fatal("Expected \")\". User-defined functions cannot have arguments.");
         return;
      }
      {  // Run additional checks on the function name.
         if (this->lookup_user_defined_function(name))
            this->throw_error(QString("A user-defined function named \"%1\" is already in scope. Functions cannot shadow each other.").arg(name));
         //
         // Do not allow user-defined functions to shadow built-ins:
         //
         OpcodeArgTypeRegistry::type_list_t sources;
         auto built_in_type = Compiler::check_name_is_taken(name, sources);
         switch (built_in_type) {
            case name_source::none:
               break;
            case name_source::action:
            case name_source::condition:
               this->throw_error(QString("User-defined functions cannot shadow built-in functions such as %1.").arg(name));
               break;
            case name_source::static_typename:
            case name_source::variable_typename:
               this->throw_error(QString("User-defined functions cannot shadow built-in type names such as %1.").arg(name));
               break;
            case name_source::namespace_member:
            case name_source::imported_name:
               this->throw_error(QString("User-defined functions cannot shadow built-in values such as %1.").arg(name));
               break;
         }
      }
      //
      auto item = new Script::Block;
      item->type = Script::Block::Type::function;
      item->name = name;
      item->set_start(start);
      item->event = this->next_event;
      this->next_event = Script::Block::Event::none;
      this->block->insert_item(item);
      this->_openBlock(item);
      //
      // Normally, we only create a Block's Trigger when we're compiling the block in full. However, 
      // in order to compile calls to this user-defined function as we find them, we need to know 
      // the index of the function's trigger... which means that the function needs to *have* a 
      // trigger. So, we'll create it early.
      //
      item->trigger = new Trigger;
      this->functions_in_scope.push_back(Script::UserDefinedFunction(name, this->results.triggers.size(), item)); // remember this function and its index
      this->results.triggers.push_back(item->trigger);
   }
   void Compiler::_handleKeyword_On() {
      if (this->block != this->root) {
         this->raise_error("Only top-level (non-nested) blocks can be event handlers.");
         this->skip_to(':');
         return;
      }
      QString words;
      auto    prior = this->backup_stream_state();
      while (!this->extract_specific_char(':')) {
         auto w = this->extract_word();
         if (w.isEmpty()) {
            this->restore_stream_state(prior);
            this->throw_error("No valid event name specified.");
         }
         if (!words.isEmpty())
            words += ' ';
         words += w;
      }
      if (words.isEmpty()) {
         this->restore_stream_state(prior);
         this->throw_error("No valid event name specified.");
      }
      auto event = Script::Block::Event::none;
      if (words == "init") {
         this->next_event = Script::Block::Event::init;
         return;
      }
      if (words == "local init") {
         this->next_event = Script::Block::Event::local_init;
         return;
      }
      if (words == "host migration") {
         this->next_event = Script::Block::Event::host_migration;
         return;
      }
      if (words == "double host migration") {
         this->next_event = Script::Block::Event::double_host_migration;
         return;
      }
      if (words == "object death") {
         this->next_event = Script::Block::Event::object_death;
         return;
      }
      if (words == "local") {
         this->next_event = Script::Block::Event::local;
         return;
      }
      if (words == "pregame") {
         this->next_event = Script::Block::Event::pregame;
         return;
      }
      this->raise_error(prior, QString("Invalid event name: \"%s\".").arg(words));
   }
}