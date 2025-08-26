#ifndef AT_TOKEN_H_
#define AT_TOKEN_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum TokenMatchStatus { TOKEN_MATCH_SUCCESS, TOKEN_MATCH_INCOMPLETE, TOKEN_MATCH_FAILURE } TokenMatchStatus;

#define TOKEN(str) {(str), (sizeof(str) - 1), false}
#define TOKEN_CI(str) {(str), (sizeof(str) - 1), true}
#define TOKEN_MATCHER(str) {&(const TokenTypeDef)TOKEN(str), 0, NULL}
#define TOKEN_MATCHER_CI(str) {&(const TokenTypeDef)TOKEN_CI(str), 0, NULL}
#define TOKEN_MATCHER_ALIGNED(str, alignedPosPtr) {&(const TokenTypeDef)TOKEN(str), 0, alignedPosPtr}
#define TOKEN_MATCHER_CI_ALIGNED(str, alignedPosPtr) {&(const TokenTypeDef)TOKEN_CI(str), 0, alignedPosPtr}

typedef struct TokenTypeDef {
	const char	  *str;
	const uint16_t length;
	const bool	   caseInsensitive;
} TokenTypeDef;

typedef struct TokenMatcher {
	const TokenTypeDef *token;
	uint16_t			position;
	uint16_t		   *alignedPosition;
} TokenMatcher;

void			 token_reset(TokenMatcher *handle);
TokenMatchStatus token_match(TokenMatcher *tokenHandle, char ch);

#endif /* AT_TOKEN_H_ */