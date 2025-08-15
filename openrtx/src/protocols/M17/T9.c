//--------------------------------------------------------------------
// T9 text entry - T9.c
//
// Wojciech Kaczmarski, SP5WWP
// M17 Project, 6 November 2024
//--------------------------------------------------------------------
#include <M17/T9.h>

uint8_t getDigit(const char c)
{
    return "22233344455566677778889999"[c-'a'];
}

char* getWord(char *dict, char *code)
{
    char *word = dict;

    uint8_t code_len=strlen(code);

    //count asterisks
    uint8_t depth=0;
    for(uint8_t i=0; i<code_len; i++)
    {
        if(code[i]=='*')
            depth++;
    }

    //subtract the amount of asterisks from the code length
    code_len-=depth;

    do
    {
        if(strlen(word)==code_len)
        {
            //speed up the search a bit. TODO: there's room for improvement here
            if(getDigit(word[0])>code[0])
            {
                break;
            }

            uint8_t sum=0;

            for(uint8_t i=0; i<code_len && sum==0; i++)
            {
                sum=getDigit(word[i])-code[i];
            }

            if(sum==0)
            {
                if(depth==0)
                    return word;
                depth--;
            }
        }

        word += strlen(word)+1;
    } while(strlen(word)!=0);

    return "";
}
