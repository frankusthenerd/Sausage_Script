// ============================================================================
// Sausage Script (Definitions)
// Programmed by Francois Lamini
// ============================================================================

#include "..\Code_Helper\Codeloader.hpp"
#include "..\Code_Helper\Allegro.hpp"

namespace Codeloader {

  class cSource {

    public:
      cArray<sToken> tokens;
      cHash<std::string, int> vars;
      cHash<std::string, cList> lists;
      cHash<std::string, cMatrix> matrices;
      cHash<std::string, std::string> strings;
      cArray<int> stack;
      cHash<std::string, int> symtab;
      int pointer;
      int status;
      cIO_Control* io;

      cSource(std::string source, cIO_Control* io);
      void Parse_Tokens(std::string source);
      void Generate_Parse_Error(std::string message, sToken token);
      void Run(int timeout);
      sToken& Get_Token();
      sToken& Peek_Token();
      void Check_Keyword(std::string keyword);
      int Eval_Expression();
      bool Eval_Condition();
      int Eval_Conditional();
      int Eval_Operand();
      bool Is_Operator(sToken& token);
      bool Is_Logic(sToken& token);
      void Find_End_Token();
      void Find_Subroutine(std::string name);
      void Interpret();
      void Store(int number, sToken& location);

  };

}
