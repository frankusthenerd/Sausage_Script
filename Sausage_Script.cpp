// ============================================================================
// Sausage Script (Implementation)
// Programmed by Francois Lamini
// ============================================================================

#include "Sausage_Script.h"

Codeloader::cSource* source = NULL;

bool Source_Process();
bool Process_Keys();

// **************************************************************************
// Program Entry Point
// **************************************************************************

int main(int argc, char** argv) {
  if (argc == 2) {
    std::string program = argv[1];
    try {
      Codeloader::cConfig config("Config");
      int width = config.Get_Property("width");
      int height = config.Get_Property("height");
      Codeloader::cAllegro_IO allegro(program, width, height, 2, "Game");
      int prgm_start = config.Get_Property("program");
      source = new Codeloader::cSource(program, &allegro);
      allegro.Load_Resources("Resources");
      allegro.Load_Button_Names("Button_Names");
      allegro.Load_Button_Map("Buttons");
      allegro.Process_Messages(Source_Process, Process_Keys);
    }
    catch (Codeloader::cError error) {
      error.Print();
    }
    if (source) {
      delete source;
    }
  }
  else {
    std::cout << "Usage: " << argv[0] << " <program>" << std::endl;
  }
  std::cout << "Done." << std::endl;
  return 0;
}

// ****************************************************************************
// Source Processor
// ****************************************************************************

/**
 * Called when command needs to be processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Source_Process() {
  source->Run(20);
  return false;
}

/**
 * Called when keys are processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Process_Keys() {
  return false;
}

namespace Codeloader {

  // **************************************************************************
  // Source Implementation
  // **************************************************************************

  /**
   * Creates a new source module.
   * @param source The source code name.
   * @param io The I/O control.
   */
  cSource::cSource(std::string source, cIO_Control* io) {
    this->pointer = 0;
    this->io = io;
    this->status = eSTATUS_IDLE;
    this->Parse_Tokens(source);
    this->status = eSTATUS_RUNNING;
  }

  /**
   * Parses tokens from a source file.
   * @param source The name of the source code.
   * @throws An error if something went wrong.
   */
  void cSource::Parse_Tokens(std::string source) {
    cFile source_file(source + ".ss");
    source_file.Read();
    int line_no = 0;
    while (!source_file.Has_More_Lines()) {
      std::string line = source_file.Get_Line();
      line_no++;
      if (line.length() > 0) {
        if (line[0] == ':') { // Code line.
          line = line.substr(1);
          // Parse the tokens.
          cArray<std::string> tokens = Parse_C_Lesh_Line(line);
          int tok_count = tokens.Count();
          for (int tok_index = 0; tok_index < tok_count; tok_index++) {
            sToken token;
            token.line_no = line_no;
            token.source = source;
            token.token = tokens[tok_index];
            this->tokens.Add(token);
          }
        }
      }
    }
  }

  /**
   * Generates a parse error.
   * @param message The error message.
   * @param token The associted token.
   * @throws An error.
   */
  void cSource::Generate_Parse_Error(std::string message, sToken token) {
    throw cError("Error: " + message + "\nLine No: " + Number_To_Text(token.line_no) + "\nSource: " + token.source + "\nToken: " + token.token);
  }

  /**
   * Runs the program.
   * @throws An error if there is an illegal command.
   */
  void cSource::Run(int timeout) {
    auto start = std::chrono::system_clock::now();
    while (this->status != eSTATUS_DONE) {
      auto end = std::chrono::system_clock::now();
      auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      if (diff.count() < timeout) {
        this->Interpret();
      }
      else {
        break;
      }
    }
  }

  /**
   * Gets a token from the token stack but does not remove it.
   * @return The token at the pointer.
   * @throws An error if there are no more tokens.
   */
  sToken& cSource::Get_Token() {
    if (this->pointer >= this->tokens.Count()) {
      throw cError("No more tokens left!");
    }
    return this->tokens[this->pointer++];
  }

  /**
   * Evaluates an expression and returns the result.
   * @return The value of the result.
   */
  int cSource::Eval_Expression() {
    int result = this->Eval_Operand();
    sToken oper = this->Peek_Token();
    while (this->Is_Operator(oper)) {
      oper = this->Get_Token();
      int value = this->Eval_Operand();
      if (oper.token == "+") {
        result += value;
      }
      else if (oper.token == "-") {
        result -= value;
      }
      else if (oper.token == "*") {
        result *= value;
      }
      else if (oper.token == "/") {
        result /= value;
      }
      else if (oper.token == "rem") {
        result %= value;
      }
      else if (oper.token == "rand") {
        result = this->io->Get_Random_Number(result, value);
      }
      else if (oper.token == "cos") {
        result = (int)((double)result * std::cos((double)value * 3.14 / 180.0));
      }
      else if (oper.token == "sin") {
        result = (int)((double)result * std::sin((double)value * 3.14 / 180.0));
      }
      oper = this->Peek_Token();
    }
    return result;
  }

  /**
   * Pulls a token but does not increment the pointer.
   * @return The token.
   * @throws An error if there is no token.
   */
  sToken& cSource::Peek_Token() {
    if (this->pointer >= this->tokens.Count()) {
      throw cError("No more tokens left!");
    }
    return this->tokens[this->pointer];
  }

  /**
   * Checks to see if a keyword exists.
   * @param keyword The keyword to check.
   * @throws An error if the keyword does not exist.
   */
  void cSource::Check_Keyword(std::string keyword) {
    sToken token = this->Get_Token();
    if (token.token != keyword) {
      this->Generate_Parse_Error("Keyword " + keyword + " missing.", token);
    }
  }

  /**
   * Determines if a token is conditional logic.
   * @param token The token to check.
   * @return True if the token is logic, false otherwise.
   */
  bool cSource::Is_Logic(sToken& token) {
    return ((token.token == "and") || (token.token == "or"));
  }

  /**
   * Evaluates a condition.
   * @return True if the condition evaluated to true, false otherwise.
   */
  bool cSource::Eval_Condition() {
    bool result = false;
    int left_result = this->Eval_Expression();
    sToken test = this->Get_Token();
    int right_result = this->Eval_Expression();
    int diff = (left_result - right_result);
    if (test.token == "=") {
      result = (diff == 0);
    }
    else if (test.token == "not") {
      result = (diff != 0);
    }
    else if (test.token == ">") {
      result = (diff > 0);
    }
    else if (test.token == "<") {
      result = (diff < 0);
    }
    else if (test.token == ">or=") {
      result = (diff >= 0);
    }
    else if (test.token == "<or=") {
      result = (diff <= 0);
    }
    else {
      this->Generate_Parse_Error("Invalid test " + test.token + ".", test);
    }
    return result;
  }

  /**
   * Evalulates a conditional.
   * @return The result of the conditional.
   */
  int cSource::Eval_Conditional() {
    int result = this->Eval_Condition();
    sToken logic = this->Peek_Token();
    while (this->Is_Logic(logic)) {
      logic = this->Get_Token();
      bool test = this->Eval_Condition();
      if (logic.token == "and") {
        result *= test; // Multiply to AND.
      }
      else if (logic.token == "or") {
        result += test; // Add to OR.
      }
      logic = this->Peek_Token();
    }
    return result;
  }

  /**
   * Evaluates the operand.
   * @return The operand which is numeric.
   * @throws An error if something went wrong.
   */
  int cSource::Eval_Operand() {
    int value = 0;
    sToken token = this->Get_Token();
    try {
      value = Text_To_Number(token.token);
    }
    catch (cError error) { // Probably a placeholder.
      if (token.token.find("->") != std::string::npos) {
        cArray<std::string> parts = Parse_Sausage_Text(token.token, "->");
        if (parts.Count() == 2) { // List
          std::string name = parts[0];
          std::string var = parts[1];
          if (this->lists.Does_Key_Exist(name)) {
            if (this->vars.Does_Key_Exist(var)) {
              int index = this->vars[var];
              value = this->lists[name][index];
            }
            else {
              this->Generate_Parse_Error("Could not find index variable for list.", token);
            }
          }
          else {
            this->Generate_Parse_Error("Could not find list " + name + ".", token);
          }
        }
        else if (parts.Count() == 3) { // Matrix
          std::string name = parts[0];
          std::string var_y = parts[1];
          std::string var_x = parts[2];
          if (this->matrices.Does_Key_Exist(name)) {
            if (this->vars.Does_Key_Exist(var_y)) {
              if (this->vars.Does_Key_Exist(var_x)) {
                int y = this->vars[var_y];
                int x = this->vars[var_x];
                value = this->matrices[name][y][x];
              }
              else {
                this->Generate_Parse_Error("Could not find x variable for matrix.", token);
              }
            }
            else {
              this->Generate_Parse_Error("Could not find y variable for matrix.", token);
            }
          }
          else {
            this->Generate_Parse_Error("Could not find matrix " + name + ".", token);
          }
        }
        else {
          this->Generate_Parse_Error("Invalid dimension type.", token);
        }
      }
      else {
        if (this->vars.Does_Key_Exist(token.token)) {
          value = this->vars[token.token];
        }
        else if (this->symtab.Does_Key_Exist(token.token)) {
          value = this->symtab[token.token];
        }
        else {
          this->Generate_Parse_Error("Symbol " + token.token + " was not found.", token);
        }
      }
    }
    return value;
  }

  /**
   * Determines if a token is an operator.
   * @param token The token to check.
   * @return True if the token is an operator, false otherwise.
   */
  bool cSource::Is_Operator(sToken& token) {
    return ((token.token == "+") ||
            (token.token == "-") ||
            (token.token == "*") ||
            (token.token == "/") ||
            (token.token == "rem") ||
            (token.token == "rand") ||
            (token.token == "cos") ||
            (token.token == "sin"));
  }

  /**
   * Finds the end token in the block.
   * @throws An error if something is missing.
   */
  void cSource::Find_End_Token() {
    int count = this->tokens.Count();
    bool found = false;
    while (this->pointer < count) {
      sToken& token = this->Get_Token();
      if ((token.token == "end") || (token.token == "else")) {
        found = true;
        break; // Jump out!
      }
      else if ((token.token == "if") || (token.token == "while") || (token.token == "subroutine") || (token.token == "object") || (token.token == "map")) {
        this->Find_End_Token();
      }
    }
    if (!found) {
      throw cError("Could not find end token!");
    }
  }

  /**
   * Finds a subroutine given the name.
   * @param name The name of the subroutine.
   * @throws An error if the subroutine could not be found.
   */
  void cSource::Find_Subroutine(std::string name) {
    int count = this->tokens.Count();
    bool found = false;
    while (this->pointer < count) {
      sToken& token = this->Get_Token();
      if (token.token == "subroutine") {
        sToken& sub_name = this->Get_Token();
        if (sub_name.token == name) {
          found = true;
          break;
        }
      }
    }
  }

  /**
   * Interprets a single command.
   * @throws An error if something went wrong.
   */
  void cSource::Interpret() {
    int command_pos = this->pointer;
    sToken& command = this->Get_Token();
    if (command.token == "if") {
      int result = this->Eval_Conditional();
      this->Check_Keyword("then");
      if (!result) {
        this->Find_End_Token();
      }
    }
    else if (command.token == "else") {
      this->Find_End_Token();
    }
    else if (command.token == "end") {
      if (this->stack.Count() > 0) {
        this->pointer = this->stack.Pop(); // Jump to saved position.
      }
    }
    else if (command.token == "while") {
      int result = this->Eval_Conditional();
      this->Check_Keyword("do");
      if (result) {
        this->stack.Push(command_pos); // Save start position of while.
      }
      else {
        this->Find_End_Token();
      }
    }
    else if (command.token == "subroutine") {
      this->Find_End_Token(); // We jump over subroutines.
    }
    else if (command.token == "call") {
      sToken& name = this->Get_Token();
      this->stack.Push(this->pointer); // Save the return address.
      this->Find_Subroutine(name.token);
    }
    else if (command.token == "store") {
      int result = this->Eval_Expression();
      this->Check_Keyword("in");
      sToken& location = this->Get_Token();
      this->Store(result, location);
    }
    else if (command.token == "output") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("at");
      int x = this->Eval_Expression();
      int y = this->Eval_Expression();
      this->Check_Keyword("color");
      int red = this->Eval_Expression();
      int green = this->Eval_Expression();
      int blue = this->Eval_Expression();
      if (this->strings.Does_Key_Exist(name.token)) {
        this->io->Output_Text(this->strings[name.token], x, y, red, green, blue);
      }
      else {
        this->Generate_Parse_Error("String " + name.token + " was not found.", name);
      }
    }
    else if (command.token == "number") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("at");
      int x = this->Eval_Expression();
      int y = this->Eval_Expression();
      this->Check_Keyword("color");
      int red = this->Eval_Expression();
      int green = this->Eval_Expression();
      int blue = this->Eval_Expression();
      if (this->vars.Does_Key_Exist(name.token)) {
        this->io->Output_Text(Number_To_Text(this->vars[name.token]), x, y, red, green, blue);
      }
      else {
        this->Generate_Parse_Error("String " + name.token + " was not found.", name);
      }
    }
    else if (command.token == "define") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("as");
      sToken& value = this->Get_Token();
      this->symtab[name.token] = Text_To_Number(value.token);
    }
    else if (command.token == "object") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("as");
      sToken value = this->Get_Token();
      int index = 0;
      while (value.token != "end") {
        this->symtab[name.token + ":" + value.token] = index++;
      }
    }
    else if (command.token == "map") {
      sToken value = this->Get_Token();
      int index = 0;
      while (value.token != "end") {
        this->symtab[value.token] = index++;
      }
    }
    else if (command.token == "var") {
      sToken& name = this->Get_Token();
      this->vars[name.token] = 0;
    }
    else if (command.token == "list") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("size");
      int size = this->Eval_Expression();
      this->lists[name.token] = cList(size);
    }
    else if (command.token == "matrix") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("size");
      int width = this->Eval_Expression();
      int height = this->Eval_Expression();
      this->matrices[name.token] = cMatrix(width, height);
    }
    else if (command.token == "string") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("as");
      sToken& string = this->Get_Token();
      this->strings[name.token] = C_Lesh_String_To_Cpp_String(string.token);
    }
    else if (command.token == "load") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("from");
      sToken& file = this->Get_Token();
      if (this->matrices.Does_Key_Exist(name.token)) {
        this->io->Load(C_Lesh_String_To_Cpp_String(file.token), this->matrices[name.token]);
      }
      else {
        this->Generate_Parse_Error("Matrix " + name.token + " does not exist.", name);
      }
    }
    else if (command.token == "save") {
      sToken& file = this->Get_Token();
      this->Check_Keyword("to");
      sToken& name = this->Get_Token();
      if (this->matrices.Does_Key_Exist(name.token)) {
        this->io->Save(C_Lesh_String_To_Cpp_String(file.token), this->matrices[name.token]);
      }
      else {
        this->Generate_Parse_Error("Matrix " + name.token + " does not exist.", name);
      }
    }
    else if (command.token == "draw") {
      sToken& name = this->Get_Token();
      this->Check_Keyword("at");
      int x = this->Eval_Expression();
      int y = this->Eval_Expression();
      int width = this->Eval_Expression();
      int height = this->Eval_Expression();
      this->Check_Keyword("angle");
      int angle = this->Eval_Expression();
      this->Check_Keyword("flip");
      int flip_x = this->Eval_Expression();
      int flip_y = this->Eval_Expression();
      this->io->Draw_Image(name.token, x, y, width, height, angle, (bool)flip_x, (bool)flip_y);
    }
    else if (command.token == "sound") {
      sToken& name = this->Get_Token();
      this->io->Play_Sound(name.token);
    }
    else if (command.token == "music") {
      sToken& name = this->Get_Token();
      this->io->Play_Music(name.token);
    }
    else if (command.token == "silence") {
      this->io->Silence();
    }
    else if (command.token == "refresh") {
      this->io->Refresh();
    }
    else if (command.token == "color") {
      int red = this->Eval_Expression();
      int green = this->Eval_Expression();
      int blue = this->Eval_Expression();
      this->io->Color(red, green, blue);
    }
    else if (command.token == "getkey") {
      sSignal signal = this->io->Read_Signal();
      sToken& location = this->Get_Token();
      this->Store(signal.code, location);
    }
    else if (command.token == "stop") {
      this->status = eSTATUS_DONE;
    }
    else {
      this->Generate_Parse_Error("Invalid command " + command.token + ".", command);
    }
  }

  /**
   * Stores a value at a location.
   * @param number The number to store.
   * @param location The location to store at.
   * @throws An error if the location is invalid.
   */
  void cSource::Store(int number, sToken& location) {
    if (location.token.find("->") != std::string::npos) {
      cArray<std::string> parts = Parse_Sausage_Text(location.token, "->");
      if (parts.Count() == 2) { // List
        std::string name = parts[0];
        std::string var = parts[1];
        if (this->lists.Does_Key_Exist(name)) {
          if (this->vars.Does_Key_Exist(var)) {
            int index = this->vars[var];
            this->lists[name][index] = number;
          }
          else {
            this->Generate_Parse_Error("Could not find index variable for list.", location);
          }
        }
        else {
          this->Generate_Parse_Error("Could not find list " + name + ".", location);
        }
      }
      else if (parts.Count() == 3) { // Matrix
        std::string name = parts[0];
        std::string var_y = parts[1];
        std::string var_x = parts[2];
        if (this->matrices.Does_Key_Exist(name)) {
          if (this->vars.Does_Key_Exist(var_y)) {
            if (this->vars.Does_Key_Exist(var_x)) {
              int y = this->vars[var_y];
              int x = this->vars[var_x];
              this->matrices[name][y][x] = number;
            }
            else {
              this->Generate_Parse_Error("Could not find x variable for matrix.", location);
            }
          }
          else {
            this->Generate_Parse_Error("Could not find y variable for matrix.", location);
          }
        }
        else {
          this->Generate_Parse_Error("Could not find matrix " + name + ".", location);
        }
      }
      else {
        this->Generate_Parse_Error("Invalid dimension type.", location);
      }
    }
    else {
      if (this->vars.Does_Key_Exist(location.token)) {
        this->vars[location.token] = number;
      }
      else if (this->symtab.Does_Key_Exist(location.token)) {
        this->symtab[location.token] = number;
      }
      else {
        this->Generate_Parse_Error("Symbol " + location.token + " was not found.", location);
      }
    }
  }

}
