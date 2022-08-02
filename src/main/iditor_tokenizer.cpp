#include "iditor_tokenizer.h"
#include <ctype.h>

int tokenizer_xTokenize(Fts5Tokenizer *,
                        void *pCtx,
                        int flags,            /* Mask of FTS5_TOKENIZE_* flags */
                        const char *pText, int nText,
                        int (*xToken)(
                            void *pCtx,         /* Copy of 2nd argument to xTokenize() */
                            int tflags,         /* Mask of FTS5_TOKEN_* flags */
                            const char *pToken, /* Pointer to buffer containing token */
                            int nToken,         /* Size of token in bytes */
                            int iStart,         /* Byte offset of token within input text */
                            int iEnd            /* Byte offset of end of token within input text */
                        )
)
{
  if (flags == FTS5_TOKENIZE_QUERY)
  {
    xToken(pCtx, 0, pText, nText, 0, nText);
    return SQLITE_OK;
  }

  for (int i = 0; i < nText; i++)
  {
    char c = tolower(pText[i]);
    xToken(pCtx, 0, &c, 1, 0, 1);
  }
  return SQLITE_OK;
}

int tokenizer_xCreate(void *, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
  _tokenizer_instance.xTokenize = tokenizer_xTokenize;
  *ppOut = (Fts5Tokenizer*)(&_tokenizer_instance);
  return 0;
}