#include "sqlite3.h"

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
);


static fts5_tokenizer _tokenizer_instance;

int tokenizer_xCreate(void *, const char **azArg, int nArg, Fts5Tokenizer **ppOut);