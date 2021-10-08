#include <Arduino.h>

#define MAX_EXPRESSION_LENGTH 15
#define MAX_DICTIONARY_CUSTOM_LENGTH 20
#define MAX_DICTIONARY_TOTAL_LENGTH MAX_DICTIONARY_CUSTOM_LENGTH + 256
#define MAX_BINARY_PATH 128
#define defaultKey "3274727565\n3266616c7365\n1e2c5c6e"
/*
OBS:
O objetivo é realizar o PACK e o UNPACK de uma string de dados em formato HEXADECIMAL criptografado em com tamanho variável.
A chave de compressão / descompressão é dada por uma árvore binária (huffmann) com símbolos e expressões.
A chave possui dupla função
1) gerar uma árvore de huffman para compressão
2) criar uma árvore diferente em função da chave para expressões mais usadas, agregando segurança com a criptografia dos dados.


As informações geradas pelo PACK sempre serão um número par de caracteres hexadecimais (ASCII) com valores expressos em caixa baixa (caracteres 0-9, a-f).
Primeiro nibble (4 bits) é valor que informa quantos bits, no final da string de dados comprimidos, deverão ser ignorados. É um valor de 0 a 7.
O último nibble (4 bits) é um OU EXCLUSIVO de todos os demais nibbles do pacote, e serve para agregar um check com possibilidade de detectar alterações ou falhas.

A chave é uma string (também expressa dados em hexadecimal) com até MAX_DICTIONARY_CUSTOM_LENGTH expressões.
Cada expressão é, em hexadecimal, dado por um byte para o peso (2 dígitos hexadecimais), seguido da expressão em hexadecimal
Para que haja maior compressão dos dados, sugere-se que os identificadores ou expressões mais recorrentes nos dados trasmitidos sejam considerados na chave. 

Exemplo de construção da chave:

Peso:   Expressão:      Peso em hexadecimal       Expressão em hexadecimal
100 "id":               64                        226964223a
100 "name":             64                        226e616d65223a
 80 "age":              50                        22616765223a
 50 true                32                        74727565
 50 false               32                        66616c7365
 30 ,\n                 1e                        2c5c6e



arquivo chave fica com as seguintes 6 linhas, cada linha separa por um new-line (chr):

64226964223a
64226e616d65223a
5022616765223a
3274727565
3266616c7365
1e2c5c6e


USO DA CLASSE:

Cowc cowc;

métodos

cowc.key(String arquivoChave);                             // gera chave para criptografia. Caso não utilizada, será gerada usando chave padrão de baixa segurança.
String cowc.packHex(String asciiText);                     // comprime e criptografa texto inputText, retornando dados comprimidos no formato ASCII HEX.
String cowc.unpackHex(String hexPack);                     // descomprime e retorna dados no formato ASCII original



Sequencia da geração dos códigos para tradução (pack) e (unpack):

CHAVE          ---> |  Gera tabela de EXPRESSÕES --->      | Gera arvore de HUFFMAN -->       | Gera tabela de CÓDIGOS
hardCoded           |  Gerada para atribuir                | Gera árvore binária, organizando | Percorre árvore memorizando
na fonte e receptor |  peso a cada caractere ou            | itens com maior prioridade mais  | em string de 0 e 1 os códigos
da mensagem         |  expressão (fohas da árvore binária) | próxima à raiz                   | binários associados a cada expr.


regras importantes:

ao comprimir
   expressões são buscadas na chave. Caso encontrada, serão codificadas como um caractere
   em caso de não haver expressão, codificada caracter a caracter
   ao gerar sequencia binária, conta-se quantos BITS foram usados. Para ficar múltiplo de 8, bits aleatórios são adicionados no começo da sequencia binária.
   um primeiro nibble (4 bits) é acrescido no início, indicando quantos bits foram adicionados para que a sequencia ficasse múltipla de 8.
   e um ultimo nibble é acrescido a título de checksum, fazendo um "ou exclusivo" de todos os nibbles usados na mensagem


   ao descomprimir, o processo inverso deve ser realizado.
*/

class Cowc
{
private:
   String keyString = ""; // guarda chave de criptografia.

   /* TIPO - ESTRUTURA DO NODO DA ÁRVORE DE HUFFMAN*/
   typedef struct
   {
      short unsigned int expressionLength = 0;
      byte expression[MAX_EXPRESSION_LENGTH];
      unsigned long occurences = 0;
      byte *NodeLeft;  // = NULL;
      byte *NodeRigth; // = NULL;
   } huffmanNode;

   huffmanNode *huffmanTreeRoot = NULL; // APONTADOR PARA A RAÍZ DA ÁRVORE DE HUFFMAN.

   /* TABELA DE EXPERSSÕES. Inicialmente é preparada para conter as FOLHAS da árvore, para posterior construção da árvore */
   struct cowcTableStr
   {
      huffmanNode *nodePtr;
   } cowcTable[MAX_DICTIONARY_TOTAL_LENGTH];

   /* estrutura da tabela de códigos (após percorrer árvore) para tradução no PACK e UNPACK */
   struct cowcCodeTableStr
   {
      byte expression[MAX_EXPRESSION_LENGTH];
      uint8_t expressionSize = 0;
      byte code[1 + MAX_BINARY_PATH / 8];
      uint8_t codeSize = 0;
   } cowcCodeTable[MAX_DICTIONARY_TOTAL_LENGTH];

   /* FUNÇÕES DE APOIO */

   void erro(int codigo) // função para efeito de depuração - exibe código de erro
   {
      Serial.println("\nCOWC ERROR " + String(codigo) + "\n");
      /*
           101, "Problema ao inserir expressão na tabela de códigos"
           102, 
           202, "Caminho binário da árvore de simbolos é muito longo para ser registrado"
           301, "memória insuficiente"
           501,
           401, "Não há mais espaço na tabela de expressões";
           402, "expressão invalida"
           601, 
           602,  "problema ao construir arvore"
           603,  "problema ao percorrer a árvore'
           */
   }

   bool isHex(char c) // retorna TRUE se caractere recebido como parâmetro é representação HEXADECIMAL
   {
      c = toupper(c);
      if (c >= 'A' && c <= 'F')
         return (true);
      if (c >= '0' && c <= '9')
         return (true);
      return (false);
   }

   int hexToInt(char c) // Converte caractere hexadecimal para inteiro. Em caso de caractere invalido, retorna 0
   {
      if (!isHex(c))
         return (0);
      c = toupper(c);
      if (c >= 'A' && c <= 'F')
         return (c + 10 - 'A');
      else
         return (c - '0');
   }

   /* MÉTODOS PARA MANIPULAR TABELA DE CÓDIGOS */

   /* adiciona uma expressão na tabela de códigos */
   bool cowcTableAdd(byte *expression, uint8_t expressionSize, String code)
   {
      int indice = 0;
      for (indice = 0; indice < MAX_DICTIONARY_TOTAL_LENGTH; indice++) // acha próxima posição livre na tabela de códigos
         if (cowcCodeTable[indice].expressionSize <= 0)
            break;
      if (indice >= MAX_DICTIONARY_TOTAL_LENGTH)
      {
         erro(101);
         return (false); // não há como adicionar símbolo...
      }
      for (int i = 0; i < expressionSize && i < MAX_EXPRESSION_LENGTH; i++)
         cowcCodeTable[indice].expression[i] = expression[i]; // copia expressão recebida como parâmetro para tabela
      cowcCodeTable[indice].expressionSize = expressionSize;  // copia tamanho da expressão
      cowcCodeTable[indice].codeSize = code.length();         // registra tamanho do código binário gerado
      if (code.length() > MAX_BINARY_PATH)
      {
         erro(102);
         return (false);
      } // tamanho binário maior do que o que pode ser registrado

      /* abaixo, registra o código binário em até 16 bytes - 128 bits */
      int bitNumber = 0;
      int byteNumber = 0;
      for (int i = 0; i < code.length(); i++) // varre código procurando 0 e 1 na STRING, registrando-os na ocorrência da tabela de códigos
      {
         bitNumber = i % 8;
         byteNumber = i / 8;
         if (code[i] == '1')
            cowcCodeTable[indice].code[byteNumber] |= (0b1 << bitNumber);
         else
            cowcCodeTable[indice].code[byteNumber] &= ~(0b1 << bitNumber);
      }
      return (true);
   }

   /* FUNÇÕES PARA ÁRVORE DE HUFFMAN */

   /* Aloca memória para um novo NODO de huffmann */
   huffmanNode *newHuffmanNode()
   {
      huffmanNode *aux = (huffmanNode *)malloc(sizeof(huffmanNode));
      if (aux == NULL) // falhou na alocaçao do nodo
      {
         erro(301);
         return (NULL);
      }
      return (aux);
   }

   /* Cria nodo de huffman */
   huffmanNode *createNode(int occurences /*peso*/,
                           byte *expression /*experssao*/,
                           int expressionLength /*tamanho da expressão*/,
                           huffmanNode *left /*apontador para próximo nodo a esquerda*/,
                           huffmanNode *rigth /*apontador para próximo nodo a direita*/)
   {
      huffmanNode *aux = newHuffmanNode();
      if (aux == NULL)
         return (NULL);
      aux->occurences = occurences;
      aux->expressionLength = expressionLength;
      aux->NodeLeft = (byte *)left;
      aux->NodeRigth = (byte *)rigth;
      for (int i = 0; i < expressionLength && i < MAX_EXPRESSION_LENGTH; i++)
         aux->expression[i] = expression[i];
      return (aux);
   }

   /* FUNÇÕES DE PREPARAÇÃO DA TABELA DE EXPRESSÕES */
   bool insertExpressionOnTable(String linha) // interpreta uma linha da string chave e insere a expressão e peso na tabela
   {
      int lineIndex;
      byte expression[MAX_EXPRESSION_LENGTH];
      for (lineIndex = 0; lineIndex < MAX_DICTIONARY_CUSTOM_LENGTH; lineIndex++)
      {
         if (cowcTable[lineIndex].nodePtr == NULL)
            break;
      }
      if (lineIndex >= MAX_DICTIONARY_CUSTOM_LENGTH)
      {
         erro(401);
         return (false);
      }

      if (linha.length() < 2)
      {
         erro(402);
         return false;
      }
      int taxa = 0;
      taxa = hexToInt(linha[0]) * 16 + hexToInt(linha[1]);
      int pos = 0;
      for (int i = 2; i < linha.length(); i++)
      {
         byte b = (byte)(hexToInt(linha[i]) * 16 + hexToInt(linha[i + 1]));
         i++;
         if (pos < MAX_EXPRESSION_LENGTH)
         {
            expression[pos] = b;
            pos++;
         }
      }
      cowcTable[lineIndex].nodePtr = createNode(taxa, expression, pos, NULL, NULL);
      if (cowcTable[lineIndex].nodePtr == NULL)
         return (false);
      return (true);
   }

   /* INSERE CARACTERES ASCII  */
   bool insertCharacterOnTable(char c, int weigth) // insere caractere na tabela com peso específico
   {
      int lineIndex;
      for (lineIndex = 0; lineIndex < MAX_DICTIONARY_TOTAL_LENGTH; lineIndex++)
      {
         if (cowcTable[lineIndex].nodePtr == NULL)
            break;
      }
      if (lineIndex >= MAX_DICTIONARY_TOTAL_LENGTH)
      {
         erro(501);
         return (false);
      }
      cowcTable[lineIndex].nodePtr = createNode(weigth, (byte *)&c, 1, NULL, NULL); // cria item na tabela de expressões
      if (cowcTable[lineIndex].nodePtr == NULL)
         return (false);
      return (true);
   }

   /* ATRIBUI PESOS PARA CARACTERES */
   int weigthChar(char c) // retorna peso para caracteres, de maneira pré-definida.
   {
      String caracteresMaisUsados = " :\"'.,\nIOUHLNRST0123456789ABCDEF";
      int peso = 0;
      if (caracteresMaisUsados.indexOf(toupper(c)) >= 0)
         peso = 10000;
      else if (c >= 'a' && c <= 'z')
         peso = 1000; // caracter minúsculo
      else if (c >= ' ' && c <= '~')
         peso = 200; // printable
      else
         peso = 1;
      return (peso);
   }

   huffmanNode *separaNodoValidoMenorValor() // descopbre qual o nodo válido com menor ocorrência
   {
      static int conta = 0;
      uint32_t menorOcc = 0xffffffff;
      int menorIndice = -1;
      int i = 0;
      for (i = 0; i < MAX_DICTIONARY_TOTAL_LENGTH; i++)
      {
         if (cowcTable[i].nodePtr != NULL)
         {
            if (cowcTable[i].nodePtr->occurences < menorOcc)
            {
               menorIndice = i;
               menorOcc = cowcTable[i].nodePtr->occurences;
            }
         }
      }
      if (menorIndice < 0)
         return (NULL);
      huffmanNode *N = cowcTable[menorIndice].nodePtr;
      cowcTable[menorIndice].nodePtr = NULL;
      conta++;
      return (N);
   }

   huffmanNode *buildTree() // constroi a árvore binária (huffman) com base na tabela
   {
      huffmanNode *left = NULL, *rigth = NULL, *newNode = NULL;
      while (true)
      {
         left = separaNodoValidoMenorValor();
         rigth = separaNodoValidoMenorValor();

         if (left == NULL)
         {
            erro(601);
            return (NULL);
         }
         if (rigth == NULL)
         {
            return (left);
         } // terminou de gerar arvore. Retorna ponteiro para nó raiz

         newNode = createNode(left->occurences + rigth->occurences, NULL, 0, left, rigth);
         bool fez = false;
         for (int i = 0; i < MAX_DICTIONARY_TOTAL_LENGTH; i++)
         {
            if (cowcTable[i].nodePtr == NULL)
            {
               cowcTable[i].nodePtr = newNode;
               fez = true;
               break;
            }
         }
         if (!fez)
         {
            erro(602);
            return (NULL);
         }
      }
   }

   void makeCodeTable(huffmanNode *H, String c) // funçao recursiva para construir a string binária (percorrer a árvore) gerando a tabela de códigos
   {
      if (H == NULL)
      {
         erro(603);
         return;
      }
      if ((H->NodeLeft == NULL) && (H->NodeRigth == NULL)) // folha
      {
         cowcTableAdd(H->expression, H->expressionLength, c);
         return;
      }
      if (H->NodeLeft != NULL)
         makeCodeTable((huffmanNode *)H->NodeLeft, c + "0");
      if (H->NodeRigth != NULL)
         makeCodeTable((huffmanNode *)H->NodeRigth, c + "1");
   }

   void generateTable() // gera tabela de expressões, com a parte "custom" da chave, e todos os demais símbolos ASCII
   {
      String line[MAX_DICTIONARY_CUSTOM_LENGTH];
      int tableLine = 0;
      // parte CUSTOM
      for (int i = 0; i < MAX_DICTIONARY_CUSTOM_LENGTH; i++)
         line[i] = "";
      // extrai da chave, separando por linhas
      for (int i = 0; i < keyString.length(); i++)
      {
         if (keyString[i] == '\n')
            tableLine++;
         if (isHex(keyString[i]))
            line[tableLine] += keyString[i];
      }
      // insere na tabela a linha da chave
      for (int i = 0; i < tableLine; i++)
         insertExpressionOnTable(line[i]);

      // insere na tabela os demais símbolos ASCII (256, de 00000000 a 11111111)
      char c = 0;
      for (int i = 0; i < 256; i++)
      {
         insertCharacterOnTable(c, weigthChar(c));
         c++;
      }
   }

   String printable(char c) // retorna versão "imprimível" do caractere
   {
      if (c < ' ' || c > '~')
         return ("char(" + String((int)c) + ")");
      else
         return (String(c));
   }

   void build() // constri árvore de huffman e monta tabela de códigos
   {
      huffmanTreeRoot = buildTree();
      makeCodeTable(huffmanTreeRoot, "");
   }

   int existCode(byte *expression, int size) //retorna posião de determinada expressão na tabela de códigos, se houver, ou -1 se não houver
   {
      for (int i = 0; i < MAX_DICTIONARY_TOTAL_LENGTH; i++)
      {
         if (cowcCodeTable[i].expressionSize != size)
            continue;
         if (memcmp(cowcCodeTable[i].expression, expression, size) == 0)
            return (i);
      }
      return (-1);
   }

   String getCode(int occ) // retorna código binário em string, relativo a posicao específica da tabela
   {
      String ret = "";
      for (int i = 0; i < cowcCodeTable[occ].codeSize; i++)
      {
         if ((cowcCodeTable[occ].code[i / 8] & (0b01 << i % 8)) > 0)
            ret += "1";
         else
            ret += "0";
      }
      return (ret);
   }

   String binaryTextToHexText(String binary) // retorna conversão de string de 0 e 1 em string com reresentação HEX, adicionando primeiro e último nibble
   {
      String output = "";
      int contabit = 0;
      int nibblesGerados = 0;
      int bitsAdicionadosNoComeco = 0;
      //           Serial.println("Binário anterior : " + binary);
      while (binary.length() % 8 != 0)
      {
         String bit;
         if (random(2))
            bit = '0';
         else
            bit = '1';
         bitsAdicionadosNoComeco++;
         binary = bit + binary;
      }
      //           Serial.println("Binário posterior: " + binary);
      while (contabit < binary.length())
      {
         byte nibble = 0;
         if (binary.length() > contabit && binary[contabit] == '1')
            nibble += 8;
         contabit++;
         if (binary.length() > contabit && binary[contabit] == '1')
            nibble += 4;
         contabit++;
         if (binary.length() > contabit && binary[contabit] == '1')
            nibble += 2;
         contabit++;
         if (binary.length() > contabit && binary[contabit] == '1')
            nibble += 1;
         contabit++;
         output += String(nibble, HEX);
         nibblesGerados++;
      }
      //            if (nibblesGerados % 2 == 1) output += 'f';
      //            byte completaOctetos = 8 - (binary.length() % 8); // quantos bits não são válidos no último pacote
      //            completaOctetos %= 8;
      //            output = String((char)('0' + completaOctetos)) + output;
      output = String((char)('0' + bitsAdicionadosNoComeco)) + output;
      byte crcNibble = 0;
      for (int i = 1; i < output.length(); i++)
         crcNibble ^= output[i];
      crcNibble %= 16;
      output += String(crcNibble, HEX);
      return (output);
   }

   String getHuffman(String S) // traduz a String de 0 e 1 recebida em texto com base na árvore de huffman.
   {
      String R = "";
      huffmanNode *N = huffmanTreeRoot;
      for (int i = 0; i < S.length(); i++)
      {
         if (S[i] == '0') // left
         {
            N = (huffmanNode *)N->NodeLeft;
         }
         else
         {
            N = (huffmanNode *)N->NodeRigth;
         }
         if (N->NodeLeft == NULL && N->NodeRigth == NULL) // folha
         {
            for (int pos = 0; pos < N->expressionLength; pos++)
               R += (char)(N->expression[pos]);
            N = huffmanTreeRoot;
         }
      }
      return (R);
   }

public:
   Cowc() // Cria COWC
   {
      for (int i = 0; i < MAX_DICTIONARY_TOTAL_LENGTH; i++)
         cowcTable[i].nodePtr = NULL;
   }

   void key(String S) // registra a chave e cria tabela de expressões, árvore de huffman e também a tabela de códigos
   {
      keyString = S;
      generateTable(); // gera tabela
      build();         // constroi árvore
   }

   String pack(String S) // comprime/codifica texto recebido, retornando string HEX
   {
      if (huffmanTreeRoot == NULL)
         key(defaultKey); // se ainda não foi apliada uma chave, aplica a chave default;
      String binary = "";
      String outputHex = "";
      byte bestExpression[MAX_EXPRESSION_LENGTH];
      int bestExpressionSize = 0;
      while (S.length() > 0)
      {
         for (bestExpressionSize = MAX_EXPRESSION_LENGTH; bestExpressionSize > 0; bestExpressionSize--)
         {
            if (S.length() < bestExpressionSize)
               bestExpressionSize = S.length();
            for (int pos = 0; pos < bestExpressionSize; pos++)
               bestExpression[pos] = S[pos];
            int occ = existCode(bestExpression, bestExpressionSize);
            if (occ >= 0)
            {
               String codigoBinario = getCode(occ);
               binary += codigoBinario;
               break;
            }
         }
         if (bestExpressionSize > 0)
            S.remove(0, bestExpressionSize);
         else
         {
            Serial.println("Problema ... expressão não codificada...");
            return ("");
         }
      }
      return (binaryTextToHexText(binary));
   }

   String unPack(String S) // descomprime/decodifica string HEX em texto
   {
      if (huffmanTreeRoot == NULL)
         key(defaultKey); // se ainda não foi apliada uma chave, aplica a chave default;
      if (S.length() <= 2)
         return (""); // pacote não tem tamanho para ser válido
      String binary = "";
      byte crcNibble = 0;
      byte crcLido = 0;
      //           byte firstNibble = S[0];
      int bitsAdicionadosNoComeco = S[0] - '0';
      byte lastNibble = tolower(S[S.length() - 1]);
      for (int i = 1; i < S.length() - 1; i++)
         crcNibble ^= S[i];
      crcNibble &= 0x0f;
      if (lastNibble >= '0' && lastNibble <= '9')
         crcLido = lastNibble - '0';
      else
         crcLido = 10 + lastNibble - 'a';
      if (crcNibble != crcLido)
      {
         Serial.println("CRC Ckeck fail! Calculado " + String(crcNibble) + " e recebido " + String(crcLido));
         return (""); // falha de CRC
      }

      if (S.length() % 2 == 1)
      {
         Serial.println("PACKED String size error!");
         return (""); // pacote não tem nro par de bytes;
      }
      for (int i = 1; i < S.length() - 1; i++)
      {
         char c = toupper(S[i]);
         switch (c)
         {
         case '0':
            binary += "0000";
            break;
         case '1':
            binary += "0001";
            break;
         case '2':
            binary += "0010";
            break;
         case '3':
            binary += "0011";
            break;
         case '4':
            binary += "0100";
            break;
         case '5':
            binary += "0101";
            break;
         case '6':
            binary += "0110";
            break;
         case '7':
            binary += "0111";
            break;
         case '8':
            binary += "1000";
            break;
         case '9':
            binary += "1001";
            break;
         case 'A':
            binary += "1010";
            break;
         case 'B':
            binary += "1011";
            break;
         case 'C':
            binary += "1100";
            break;
         case 'D':
            binary += "1101";
            break;
         case 'E':
            binary += "1110";
            break;
         case 'F':
            binary += "1111";
            break;
         default:
            return (""); // nibble inválido
         }
      }
      //         binary.remove(binary.length() - (firstNibble - '0'));
      if (bitsAdicionadosNoComeco > 0)
         binary.remove(0, bitsAdicionadosNoComeco);
      //         Serial.println("Binário refeito: " + binary);
      return (getHuffman(binary));
   }

   String printcode() // retorna tabela de códigos
   {
      String ret = "COWC Tab:\n";
      for (int i = 0; i < MAX_DICTIONARY_TOTAL_LENGTH; i++)
      {
         if (cowcCodeTable[i].expressionSize > 0)
         {
            ret += "[";
            for (int b = 0; b < cowcCodeTable[i].expressionSize; b++)
               ret += printable(cowcCodeTable[i].expression[b]);
            ret += "]:";
            ret += String(cowcCodeTable[i].codeSize);
            ret += "bits ";
            ret += getCode(i);
            ret += "\n";
         }
      }
      return (ret);
   }
};