import { Token } from './Token';

export class TokenSet {
	constructor() {
		this.xmlnsToken = this.add('xmlns');
	}

	add(name: string) {
		const token = new Token(name, ++this.lastNum);

		this.list[token.id] = token;
		return(token);
	}

	xmlnsToken: Token;
	list: Token[] = [];
	lastNum = -1;
}
