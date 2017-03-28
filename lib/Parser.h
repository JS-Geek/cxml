#pragma once

#include <memory>

#include <nbind/api.h>

#include "Namespace.h"
#include "PatriciaCursor.h"
#include "ParserConfig.h"

/** Fast streaming XML parser. */

class Parser {

public:

	/** Parser states. */

	enum class State : uint32_t {
		BEGIN,
		BEFORE_TEXT, TEXT,
		AFTER_LT,
		BEFORE_ELEMENT_NAME,
		EXPECT_ELEMENT_NAME, NAME,
		EMIT_PARTIAL_NAME, UNKNOWN_NAME,
		STORE_ELEMENT_NAME, AFTER_ELEMENT_NAME,
		AFTER_CLOSE_ELEMENT_NAME,
		AFTER_ATTRIBUTE_NAME,
		BEFORE_ATTRIBUTE_VALUE, AFTER_ATTRIBUTE_VALUE,
		DEFINE_XMLNS_PREFIX, AFTER_XMLNS_NAME,
		SGML_DECLARATION,
		AFTER_PROCESSING_NAME, AFTER_PROCESSING_VALUE,
		BEFORE_COMMENT, COMMENT, COMMENT_ENDING,
		EXPECT,
		ERROR
	};

	static constexpr unsigned int TOKEN_SHIFT = 5;

	// TODO: cdata start/end (no entity parsing on JS side)
	// Namespace shorthand declaration start/end
	// Namespace URI start / end ??? (could be handled as an attribute value)
	enum class TokenType : uint32_t {
		OPEN_ELEMENT_ID = 0,
		CLOSE_ELEMENT_ID,
		ATTRIBUTE_ID,
		PROCESSING_ID,
		XMLNS_ID,

		ATTRIBUTE_START_OFFSET,
		ATTRIBUTE_END_OFFSET,

		TEXT_START_OFFSET,
		TEXT_END_OFFSET,

		COMMENT_START_OFFSET,
		COMMENT_END_OFFSET,

		// Unrecognized element name.
		UNKNOWN_START_OFFSET,

		// The order of these must match OPEN_ELEMENT_ID, CLOSE_ELEMENT_ID...
		UNKNOWN_OPEN_ELEMENT_END_OFFSET,
		UNKNOWN_CLOSE_ELEMENT_END_OFFSET,
		UNKNOWN_ATTRIBUTE_END_OFFSET,
		UNKNOWN_PROCESSING_END_OFFSET,
		UNKNOWN_XMLNS_END_OFFSET,

		PROCESSING_END_TYPE,

		// Recognized part from an unrecognized name.
		PARTIAL_NAME_LEN,
		PARTIAL_NAME_ID
	};

	Parser(std::shared_ptr<ParserConfig> config);

	~Parser() {
		delete[] namespaceList;
	}

	/** Parse a chunk of incoming data. */
	bool parse(nbind::Buffer chunk);

	void setTokenBuffer(nbind::Buffer tokenBuffer, nbind::cbFunction &flushTokens) {
		this->flushTokens = std::unique_ptr<nbind::cbFunction>(new nbind::cbFunction(flushTokens));
		this->tokenBuffer = tokenBuffer;

		tokenList = reinterpret_cast<uint32_t *>(tokenBuffer.data());
		tokenBufferEnd = tokenList + tokenBuffer.length() / 4;
	}

	inline void flush(uint32_t *&tokenPtr) {
		(*flushTokens)();
		tokenList[0] = 0;
		tokenPtr = tokenList + 1;
	}

	/** Output a token. This is the only function writing to memory, so safety
	  * from code execution exploits depends on this and nothing else. */

	inline void writeToken(TokenType kind, uint32_t token, uint32_t *&tokenPtr) {
		if(tokenPtr >= tokenBufferEnd) flush(tokenPtr);

		// Buffer content length is stored at its beginning.
		++tokenList[0];

		// This must never write outside the range
		// from tokenList to tokenBufferEnd (exclusive).
		*tokenPtr++ = static_cast<uint32_t>(kind) + (token << TOKEN_SHIFT);
	}

	void setPrefixTrie(nbind::Buffer buffer) {
		prefixBuffer = buffer;
		prefixTrie.setRoot(buffer.data());
	}

	void debug(unsigned char c);

	std::shared_ptr<ParserConfig> config;

	/** Namespace list copied from config, which owns it. */
	const Namespace **namespaceList;

	PatriciaCursor cursor;

	State state;
	/** Next state after reading an element, attribute or processing instruction
	  * name, a text node or an attribute value. */
	State nextState;
	/** Next state if the current character was not the expected one. */
	State otherState;
	/** Next state after reading an attribute value. Regular elements and
	  * processing instructions need different handling. */
	State afterValueState;
	/** Flag whether the previously emitted name was found in a trie. */
	bool knownName;

	/** Expected character for moving to another state. */
	unsigned char expected;

	size_t pos;

	uint32_t elementID;
	TokenType tokenType;
	const unsigned char *tokenStart;

	// TODO: Maybe this could be std::function<void ()>
	std::unique_ptr<nbind::cbFunction> flushTokens;

	Patricia prefixTrie;
	nbind::Buffer prefixBuffer;
	nbind::Buffer tokenBuffer;
	uint32_t *tokenList;
	const uint32_t *tokenBufferEnd;

};