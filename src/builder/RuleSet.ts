import { SimpleSchema } from '../schema/SimpleSchema';
import { ComplexType } from '../schema/ComplexType';
import { MemberSpec } from '../schema/Member';
import { ElementInstance, ElementMeta, ElementConstructor } from '../schema/Element';

export class Rule {

	addElement(member: RuleMember) {
		this.elements[member.id] = member;
	}

	addAttribute(member: RuleMember) {
		this.attributes[member.id] = member;
	}

	elements: { [id: number]: RuleMember } = {};
	attributes: { [id: number]: RuleMember } = {};

	static string = new Rule();

	XMLType: ElementConstructor;

}

export class RuleMember {

	constructor(public rule: Rule, public spec: MemberSpec) {
		this.id = spec.meta!.token.id!;
		this.min = spec.min;
		this.max = spec.max;
	}

	id: number;
	min: number;
	max: number;

}

function link<Type>(parent: Type) {
	function Result() {}
	Result.prototype = parent;
	return(new (Result as any)());
}

export interface RuleStack {
	meta?: ElementMeta;
	rule?: Rule;
	parent?: RuleStack;
}

export class RuleSet {

	createRule(type: ComplexType, meta?: ElementMeta, parent?: RuleStack) {
		const rule = new Rule();
		let childRule: Rule | undefined;
		let proto: { [key: string]: any } = {};

		if(meta) {
			rule.XMLType = meta.createProto();
			proto = rule.XMLType.prototype;
		}

		if(type.elements && type.elements.group) {
			for(let childSpec of type.elements.group.list) {
				const memberMeta = childSpec.meta;

				if(memberMeta) {
					if(memberMeta instanceof ElementMeta) {
						childRule = void 0;

						for(let item = parent; item; item = item.parent) {
							if(item.meta == memberMeta) {
								// If the child element type matches an ancestor's,
								// re-use its rule to avoid infinite recursion.

								childRule = item.rule;
								break;
							}
						}

						if(!childRule) {
							childRule = this.createRule(memberMeta.type, memberMeta, { meta, rule, parent });
						}

						// Subclass type metadata and clear existence flag to indicate a placeholder.
						let fakeMeta = link(memberMeta);
						fakeMeta.exists = false;

						let placeholder: ElementInstance | ElementInstance[] | null = new childRule.XMLType();
						placeholder._ = fakeMeta;
						memberMeta.placeholder = placeholder;

						if(childSpec.max > 1) {
							// Use arrays as placeholders for arrays of children.
							placeholder = childSpec.min > 0 ? [ placeholder ] : [];
						} else if(childSpec.min < 1) {
							placeholder = null;
						}

						if(placeholder) {
							Object.defineProperty(proto, memberMeta.token.name, {
								configurable: true,
								enumerable: false,
								value: placeholder,
								writable: true
							});
						}
					} else childRule = Rule.string;

					rule.addElement(new RuleMember(childRule, childSpec));
				}
			}
		}

		if(type.attributes) {
			for(let attributeSpec of type.attributes.list) {
				const memberMeta = attributeSpec.meta;

				if(memberMeta) {
					// const token = memberMeta.token;

					childRule = Rule.string;

					rule.addAttribute(new RuleMember(childRule, attributeSpec));
				}
			}
		}

		return(rule);
	}

	constructor(public schema: SimpleSchema) {
		this.rootRule = this.createRule(schema.document);
	}

	rootRule: Rule;

}
