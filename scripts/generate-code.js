// This is a draft.
// It may load from sight.

class GeneratedCode {
    elements = [];
    currentFunction = undefined;
    currentStmt = undefined;

}

class GeneratedElement {
    /**
     * node id
     * @type {number}
     */
    fromNode = 0;

}

/**
 * Represent a word. e.g. varName, identifier
 */
class Token extends GeneratedElement {
    constructor(word) {
        super();

        this.word = word;
    }
}

class Literal extends GeneratedElement {
    constructor(value) {
        super();
        this.value = value;
    }
}

class IntLiteral extends Literal {

    constructor(value) {
        super(value);
    }
}

class StringLiteral extends Literal {

    constructor(value) {
        super(value);
    }
}

class CharLiteral extends Literal {

    constructor(value) {
        super(value);
    }
}

class AnnotationExpr {

    /**
     *
     * @param type
     * @param params         FieldOrParameter(name, defaultValue)
     * @param defaultValue
     */
    constructor(type, params = [], defaultValue = undefined) {
        this.type = type;
        this.params = params;
        this.defaultValue = defaultValue;
    }
}


/**
 * Used for class's fields. Function params perhaps
 */
class FieldOrParameter {
    constructor(name, type, defaultValue = undefined, annotations = []) {
        this.name = name;
        this.type = type;
        this.defaultValue = defaultValue;
        this.annotations = annotations;
    }
}

class GeneratedClass {
    fields = [];
    functions = [];

    constructor() {
    }
}

class GeneratedBlock {
    stmts = [];
}

class GeneratedFunction {
    block = new GeneratedBlock();

    constructor(name, returnType, params = [],annotations = []) {
        this.name = name;
        this.returnType = returnType;
        this.params = params;
        this.annotations = annotations;
    }

}

class FunctionCallExpr extends GeneratedElement {

    /**
     *
     * @param object    token/functionCallExpr/...
     * @param classname
     * @param name     function name
     * @param args
     */
    constructor(object, name, classname = null, args = []) {
        super();
        this.classname = classname;
        this.name = name;
        this.args = args;
        this.object = object;
    }


}

/**
 * e.g. Student::getAge
 */
class FunctionRefExpr extends FunctionCallExpr {

}

class AssignValueStmt extends GeneratedElement {
    constructor(left, right) {
        super();
        this.left = left;
        this.right = right;
    }
}

class IfStmt extends GeneratedElement {

    constructor(condition,ifBlock, elseStmt) {
        super();
        this.condition = condition;
        this.ifBlock = ifBlock;
        this.elseStmt = elseStmt;
    }
}

class ForStmt extends GeneratedElement {

}

