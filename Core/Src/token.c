#include "token.h"

#include <stddef.h>

#include "case.h"

static uint16_t get_token_alignment(const TokenMatcher *tokenHandle) {
	if (!tokenHandle) return 0;
	if (tokenHandle->alignedPosition == NULL) return tokenHandle->position;
	return *tokenHandle->alignedPosition;
}

static bool does_char_match(const TokenMatcher *tokenHandle, char ch) {
	if (!tokenHandle || !tokenHandle->token || tokenHandle->position >= tokenHandle->token->length) return false;

	if (tokenHandle->token->caseInsensitive) {
		return to_lower(tokenHandle->token->str[tokenHandle->position]) == to_lower(ch);
	}

	return tokenHandle->token->str[tokenHandle->position] == ch;
}

static TokenMatchStatus token_match_aligned_no_reset(TokenMatcher *tokenHandle, char ch) {
	if (!tokenHandle) return TOKEN_MATCH_FAILURE;

	if (tokenHandle->position != get_token_alignment(tokenHandle)) {
		return TOKEN_MATCH_FAILURE;
	}

	if (does_char_match(tokenHandle, ch)) {
		if (++tokenHandle->position == tokenHandle->token->length) {
			return TOKEN_MATCH_SUCCESS;
		}

		return TOKEN_MATCH_INCOMPLETE;
	}

	return TOKEN_MATCH_FAILURE;
}

static TokenMatchStatus token_match_aligned(TokenMatcher *tokenHandle, char ch) {
	if (!tokenHandle) return TOKEN_MATCH_FAILURE;

	TokenMatchStatus status = token_match_aligned_no_reset(tokenHandle, ch);

	if (status == TOKEN_MATCH_FAILURE) {
		token_reset(tokenHandle);  // reset the token and retry at position 0, if we're aligned to 0
		status = token_match_aligned_no_reset(tokenHandle, ch);
	}

	if (status != TOKEN_MATCH_INCOMPLETE) {
		token_reset(tokenHandle);
	}

	return status;
}

void token_reset(TokenMatcher *tokenHandle) {
	if (!tokenHandle) return;
	tokenHandle->position = 0;
}

TokenMatchStatus token_match(TokenMatcher *tokenHandle, char ch) {
	if (!tokenHandle) return TOKEN_MATCH_FAILURE;
	return token_match_aligned(tokenHandle, ch);
}