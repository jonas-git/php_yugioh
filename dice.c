// Source: http://www.melvilletheatre.com/articles/cstuff/5.html
// Modifications made by Jonas Vanen (2017)
//
// Fuzzy String Matching using Dice's Coefficient
// Based on the algorithm as described by Simon White at http://www.catalysoft.com/articles/strikeamatch.html
// 
// Copyright (c) 2013, Frank Cox
// All rights reserved.
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// THIS SOFTWARE IS PROVIDED BY FRANK COX ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANK COX BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
// OF SUCH DAMAGE.
//

#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>

struct pairs;
struct pairs {
	wchar_t chars[2];
	struct pairs *next;
};

struct match {
	struct pairs *head;
	struct pairs *current;
	struct pairs *previous;
};

typedef struct pairs pairs;
typedef struct match match;

#define INIT_MATCH(match) (match)->head = (match)->current = (match)->previous = NULL
#define INIT_PAIRS(pairs) (pairs)->next = NULL

int match_fill(match *spairs, const wchar_t *s) {
	int counter = 0, pairs_count = 0;

	while (s[counter + 1] != '\0') {
		if (s[counter] != ' ' && s[counter + 1] != ' ') {
			//INIT_MATCH(spairs);
			
			spairs->current = (pairs *)calloc(1, sizeof(pairs));
			if (!spairs->current) {
				exit(EXIT_FAILURE);
			}

			if (!spairs->head) {
				spairs->head = spairs->current;
			}
			else {
				spairs->previous->next = spairs->current;
			}

			spairs->current->next = NULL;
			spairs->current->chars[0] = toupper(s[counter]);
			spairs->current->chars[1] = toupper(s[counter + 1]);
			spairs->previous = spairs->current;
			++pairs_count;
		}
		++counter;
	}

	return pairs_count;
}

void match_free(match *spairs) {
	spairs->current = spairs->head;
	while (spairs->current != NULL) {
		pairs *next = spairs->current->next;
		free(spairs->current);
		spairs->current = next;
	}
}

double dice_coefficient(const wchar_t *sa, const wchar_t *sb) {
	if (sa[0] == '\0' || sb[0] == '\0') {
		return 0.0;
	}

	match sa_pairs, sb_pairs;
	INIT_MATCH(&sa_pairs);
	INIT_MATCH(&sb_pairs);

	int sa_pairs_count = match_fill(&sa_pairs, sa);
	int sb_pairs_count = match_fill(&sb_pairs, sb);

	double counter = 0;
	sa_pairs.current = sa_pairs.head;

	while (sa_pairs.current) {
		sb_pairs.current = sb_pairs.head;
		while (sb_pairs.current) {
			if (sb_pairs.current->chars[0] == sa_pairs.current->chars[0] && sb_pairs.current->chars[1] == sa_pairs.current->chars[1]) {
				sb_pairs.current->chars[0] = '\0';
				counter += 2.0;
				break;
			}
			sb_pairs.current = sb_pairs.current->next;
		}
		sa_pairs.current = sa_pairs.current->next;
	}

	match_free(&sa_pairs);
	match_free(&sb_pairs);

	return counter / (sa_pairs_count + sb_pairs_count);
}
