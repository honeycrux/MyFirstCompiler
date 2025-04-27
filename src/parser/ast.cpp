module;

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <sstream>
#include <variant>
#include <optional>

export module ast;

import token;
import symbol;

export struct Quadruple {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;

    std::string toString() const {
        std::ostringstream oss;
        oss << op << " " << arg1 << " " << arg2 << " " << result;
        return oss.str();
    }
};

export struct Label {
    int id;

    std::string getName() const {
        std::ostringstream oss;
        oss << "L" << id;
        return oss.str();
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << getName() << ":";
        return oss.str();
    }
};

export using Quadruples = std::vector<std::variant<Quadruple, Label>>;

export struct GeQ {
    Quadruples quads;
    std::string result;
};

export class AstNode {
    private:
        const std::string name;

    protected:
        std::string getIntermediate(int intermediateId) const {
            std::ostringstream oss;
            oss << "t" << intermediateId;
            return oss.str();
        }

        Label getLabel(int& globalLabelId) const {
            Label label{ globalLabelId };
            globalLabelId++;
            return label;
        }

        // std::string openString() const {
        //     std::ostringstream oss;
        //     oss << name << "( ";
        //     return oss.str();
        // }
        // std::string closeString() const {
        //     return " )";
        // }
        // std::string itemToString(const std::variant<Token, AstNode*>& child, const int i) const {
        //     std::ostringstream oss;
        //     if (i > 0) {
        //         oss << ", ";
        //     }
        //     if (std::holds_alternative<Token>(child)) {
        //         oss << std::get<Token>(child).toStringPrint();
        //     } else {
        //         oss << std::get<AstNode*>(child)->toString();
        //     }
        //     return oss.str();
        // }

    public:
        AstNode(const std::string& name): name(name) {}
        virtual ~AstNode() = default;

        // virtual std::string toString() const = 0;

        virtual GeQ toQuadruples(int& globalLabelId, int intermediateId = 0) const = 0;

        std::string toQuadrupleString() const {
            int globalLabelId = 0;
            const auto quads = toQuadruples(globalLabelId, 0).quads;
            std::ostringstream oss;
            for (const auto& quad : quads) {
                if (std::holds_alternative<Quadruple>(quad)) {
                    oss << std::get<Quadruple>(quad).toString() << "\n";
                } else {
                    oss << std::get<Label>(quad).toString() << "\n";
                }
            }
            return oss.str();
        }
};

export using AstChildren = std::vector<std::variant<Token, std::unique_ptr<AstNode>>>;
export using AstHandler = std::function<std::unique_ptr<AstNode>(const AstChildren& children)>;

export class DeclList : public AstNode {
    private:
        std::vector<std::unique_ptr<AstNode>> declarations;

    public:
        DeclList(std::vector<std::unique_ptr<AstNode>> declarations): AstNode("DeclList"), declarations(std::move(declarations)) {}
        ~DeclList() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& decl : declarations) {
                const auto declGeQ = decl->toQuadruples(globalLabelId);
                const auto declQuads = declGeQ.quads;
                quads.insert(quads.end(), declQuads.begin(), declQuads.end());
            }
            return { quads, "" };
        }
}; // declarations: AstNode[]

export class FuncDef : public AstNode {
    private:
        Token type;
        Token id;
        std::vector<std::unique_ptr<AstNode>> params;
        std::unique_ptr<AstNode> body;

    public:
        FuncDef(Token type, Token id, std::vector<std::unique_ptr<AstNode>> params, std::unique_ptr<AstNode> body)
            : AstNode("FuncDef"), type(type), id(id), params(std::move(params)), body(std::move(body)) {}
        ~FuncDef() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            quads.emplace_back(Quadruple{"FUNCTION", id.getValue(), std::to_string(params.size()), ""});
            for (int i = 0; i < params.size(); ++i) {
                const auto paramsGeQ = params[i]->toQuadruples(globalLabelId);
                quads.emplace_back(Quadruple{"PARAM", paramsGeQ.result, std::to_string(i + 1), ""});
            }
            const auto bodyGeQ = body->toQuadruples(globalLabelId);
            quads.insert(quads.end(), bodyGeQ.quads.begin(), bodyGeQ.quads.end());
            quads.emplace_back(Quadruple{"ENDFUNC", id.getValue(), "", ""});
            return { quads, "" };
        }
}; // type: Token, id: Token, params: AstNode[], body: AstNode

export class Param : public AstNode {
    private:
        Token type;
        Token id;
        bool array;

    public:
        Param(Token type, Token id, bool array): AstNode("Param"), type(type), id(id), array(array) {}
        ~Param() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            return { {}, id.getValue() };
        }
}; // type: Token, id: Token, array: bool

export class VarDecl : public AstNode {
    private:
        Token type;
        std::vector<std::unique_ptr<AstNode>> varAssignables;

    public:
        VarDecl(Token type, std::vector<std::unique_ptr<AstNode>> varAssignables): AstNode("VarDecl"), type(type), varAssignables(std::move(varAssignables)) {}
        ~VarDecl() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& var : varAssignables) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varQuads = varGeQ.quads;
                quads.insert(quads.end(), varQuads.begin(), varQuads.end());
            }
            return { quads, "" };
        }
}; // type: Token, varAssignables: AstNode[]

export class VarAssignable : public AstNode {
    private:
        std::unique_ptr<AstNode> var;
        std::optional<std::unique_ptr<AstNode>> expr;

    public:
        VarAssignable(std::unique_ptr<AstNode> var, std::optional<std::unique_ptr<AstNode>> expr)
            : AstNode("VarAssignable"), var(std::move(var)), expr(std::move(expr)) {}
        ~VarAssignable() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            if (expr) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varResult = varGeQ.result;

                const auto valueGeQ = (*expr)->toQuadruples(globalLabelId, intermediateId + 1);
                const auto valueQuads = valueGeQ.quads;
                const auto valueResult = valueGeQ.result;
                quads.insert(quads.end(), valueQuads.begin(), valueQuads.end());
                quads.emplace_back(Quadruple{"=", valueResult, "", varResult});
            }
            return { quads, "" };
        }
}; // var: AstNode, expr?: AstNode

export class Var : public AstNode {
    private:
        Token id;
        std::optional<Token> arrayIndex;

    public:
        Var(Token id, std::optional<Token> arrayIndex): AstNode("Var"), id(id), arrayIndex(arrayIndex) {}
        ~Var() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            if (arrayIndex) {
                return { {}, id.getValue() + "[" + arrayIndex->getValue() + "]" };
            }
            return { {}, id.getValue() };
        }
}; // id: Token, arrayIndex?: Token

export class Type : public AstNode {
    private:
        Token type;

    public:
        Type(Token type): AstNode("Type"), type(type) {}
        ~Type() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            return { {}, type.getValue() };
        }
}; // type: Token

export class Constant : public AstNode {
    private:
        Token value;

    public:
        Constant(Token value): AstNode("Constant"), value(value) {}
        ~Constant() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            return { {}, value.getValue() };
        }
}; // value: Token

export class BlockStmt : public AstNode {
    private:
        std::vector<std::unique_ptr<AstNode>> statements;

    public:
        BlockStmt(std::vector<std::unique_ptr<AstNode>> stmts): AstNode("BlockStmt"), statements(std::move(stmts)) {}
        ~BlockStmt() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& stmt : statements) {
                const auto stmtGeQ = stmt->toQuadruples(globalLabelId);
                const auto stmtQuads = stmtGeQ.quads;
                quads.insert(quads.end(), stmtQuads.begin(), stmtQuads.end());
            }
            return { quads, "" };
        }
}; // stmts: AstNode[]

export class IfStmt : public AstNode {
    private:
        std::unique_ptr<AstNode> condExpr;
        std::unique_ptr<AstNode> thenBody;
        std::optional<std::unique_ptr<AstNode>> elseBody;

    public:
        IfStmt(std::unique_ptr<AstNode> condExpr, std::unique_ptr<AstNode> thenBody, std::optional<std::unique_ptr<AstNode>> elseBody)
            : AstNode("IfStmt"), condExpr(std::move(condExpr)), thenBody(std::move(thenBody)), elseBody(std::move(elseBody)) {}
        ~IfStmt() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto label1 = getLabel(globalLabelId);
            const auto label2 = getLabel(globalLabelId);

            const auto condGeQ = condExpr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), condGeQ.quads.begin(), condGeQ.quads.end());
            quads.emplace_back(Quadruple{"if", condGeQ.result, "", label1.getName()});
            quads.emplace_back(Quadruple{"goto", "", "", label2.getName()});

            quads.emplace_back(label1);
            const auto thenGeQ = thenBody->toQuadruples(globalLabelId);
            const auto thenQuads = thenGeQ.quads;
            quads.insert(quads.end(), thenQuads.begin(), thenQuads.end());

            if (elseBody) {
                const auto label3 = getLabel(globalLabelId);
                quads.emplace_back(Quadruple{"goto", "", "", label3.getName()});

                quads.emplace_back(label2);
                const auto elseGeQ = (*elseBody)->toQuadruples(globalLabelId);
                const auto elseQuads = elseGeQ.quads;
                quads.insert(quads.end(), elseQuads.begin(), elseQuads.end());

                quads.emplace_back(label3);
            }
            else {
                quads.emplace_back(label2);
            }
            return { quads, "" };
        }
}; // condExpr: AstNode, thenBody: AstNode, elseBody: AstNode

export class WhileStmt : public AstNode {
    private:
        std::unique_ptr<AstNode> condExpr;
        std::unique_ptr<AstNode> body;

    public:
        WhileStmt(std::unique_ptr<AstNode> condExpr, std::unique_ptr<AstNode> body)
            : AstNode("WhileStmt"), condExpr(std::move(condExpr)), body(std::move(body)) {}
        ~WhileStmt() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto label1 = getLabel(globalLabelId);
            const auto label2 = getLabel(globalLabelId);
            const auto label3 = getLabel(globalLabelId);

            quads.emplace_back(label1);
            const auto condGeQ = condExpr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), condGeQ.quads.begin(), condGeQ.quads.end());
            quads.emplace_back(Quadruple{"if", condGeQ.result, "", label2.getName()});
            quads.emplace_back(Quadruple{"goto", "", "", label3.getName()});

            quads.emplace_back(label2);
            const auto bodyGeQ = body->toQuadruples(globalLabelId);
            const auto bodyQuads = bodyGeQ.quads;
            quads.insert(quads.end(), bodyQuads.begin(), bodyQuads.end());
            quads.emplace_back(Quadruple{"goto", "", "", label1.getName()});

            quads.emplace_back(label3);

            return { quads, "" };
        }
}; // condExpr: AstNode, body: AstNode

export class ForStmt : public AstNode {
    private:
        std::optional<Token> type;
        std::vector<std::unique_ptr<AstNode>> forVarDecl;
        std::unique_ptr<AstNode> condExpr;
        std::unique_ptr<AstNode> incrExpr;

    public:
        ForStmt(std::optional<Token> type, std::vector<std::unique_ptr<AstNode>> forVarDecl, std::unique_ptr<AstNode> condExpr, std::unique_ptr<AstNode> incrExpr)
            : AstNode("ForStmt"), type(type), forVarDecl(std::move(forVarDecl)), condExpr(std::move(condExpr)), incrExpr(std::move(incrExpr)) {}
        ~ForStmt() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto label1 = getLabel(globalLabelId);
            const auto label2 = getLabel(globalLabelId);
            const auto label3 = getLabel(globalLabelId);

            for (const auto& var : forVarDecl) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varQuads = varGeQ.quads;
                quads.insert(quads.end(), varQuads.begin(), varQuads.end());
            }

            quads.emplace_back(label1);
            const auto condGeQ = condExpr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), condGeQ.quads.begin(), condGeQ.quads.end());
            quads.emplace_back(Quadruple{"if", condGeQ.result, "", label2.getName()});
            quads.emplace_back(Quadruple{"goto", "", "", label3.getName()});

            quads.emplace_back(label2);
            const auto incrGeQ = incrExpr->toQuadruples(globalLabelId);
            const auto incrQuads = incrGeQ.quads;
            quads.insert(quads.end(), incrQuads.begin(), incrQuads.end());
            quads.emplace_back(Quadruple{"goto", "", "", label1.getName()});

            quads.emplace_back(label3);

            return { quads, "" };
        }

}; // type?: Token, forVarDecl: AstNode[], condExpr: AstNode, incrExpr: AstNode

export class ForVarDecl : public AstNode {
    private:
        Token type;
        std::vector<std::unique_ptr<AstNode>> varAssignables;

    public:
        ForVarDecl(Token type, std::vector<std::unique_ptr<AstNode>> varAssignables): AstNode("ForVarDecl"), type(type), varAssignables(std::move(varAssignables)) {}
        ~ForVarDecl() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (const auto& var : varAssignables) {
                const auto varGeQ = var->toQuadruples(globalLabelId);
                const auto varQuads = varGeQ.quads;
                quads.insert(quads.end(), varQuads.begin(), varQuads.end());
            }
            return { quads, "" };
        }
}; // type: Token, varAssignables: AstNode[]

export class ReturnStmt : public AstNode {
    private:
        std::optional<std::unique_ptr<AstNode>> expr;

    public:
        ReturnStmt(std::optional<std::unique_ptr<AstNode>> expr): AstNode("ReturnStmt"), expr(std::move(expr)) {}
        ~ReturnStmt() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            if (expr) {
                const auto valueGeQ = (*expr)->toQuadruples(globalLabelId, intermediateId + 1);
                quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
                quads.emplace_back(Quadruple{"RETURN", valueGeQ.result, "", ""});
            } else {
                quads.emplace_back(Quadruple{"RETURN", "", "", ""});
            }
            return { quads, "" };
        }
}; // expr?: AstNode

export class AssignExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> var;
        std::unique_ptr<AstNode> expr;

    public:
        AssignExpr(std::unique_ptr<AstNode> var, std::unique_ptr<AstNode> expr)
            : AstNode("AssignExpr"), var(std::move(var)), expr(std::move(expr)) {}
        ~AssignExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto varGeQ = var->toQuadruples(globalLabelId);
            const auto varResult = varGeQ.result;

            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto valueQuads = valueGeQ.quads;
            const auto valueResult = valueGeQ.result;
            quads.insert(quads.end(), valueQuads.begin(), valueQuads.end());
            quads.emplace_back(Quadruple{"=", valueResult, "", varResult});
            return { quads, varResult };
        }
}; // var: AstNode, expr: AstNode

export class OrExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        OrExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("OrExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~OrExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"||", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class AndExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        AndExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("AndExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~AndExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"&&", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class EqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        EqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("EqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~EqualExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"==", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class NotEqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        NotEqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("NotEqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~NotEqualExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"!=", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class LessExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        LessExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("LessExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~LessExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"<", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class LessEqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        LessEqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("LessEqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~LessEqualExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"<=", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode
export class MoreExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        MoreExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("MoreExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~MoreExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{">", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class MoreEqualExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        MoreEqualExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("MoreEqualExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~MoreEqualExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{">=", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class AddExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        AddExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("AddExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~AddExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"+", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class SubExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        SubExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("SubExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~SubExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"-", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode
export class MulExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        MulExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("MulExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~MulExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"*", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class DivExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        DivExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("DivExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~DivExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"/", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class ModExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> lexpr;
        std::unique_ptr<AstNode> rexpr;

    public:
        ModExpr(std::unique_ptr<AstNode> lexpr, std::unique_ptr<AstNode> rexpr)
            : AstNode("ModExpr"), lexpr(std::move(lexpr)), rexpr(std::move(rexpr)) {}
        ~ModExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto lexprGeQ = lexpr->toQuadruples(globalLabelId, intermediateId + 1);
            const auto rexprGeQ = rexpr->toQuadruples(globalLabelId, intermediateId + 2);
            quads.insert(quads.end(), lexprGeQ.quads.begin(), lexprGeQ.quads.end());
            quads.insert(quads.end(), rexprGeQ.quads.begin(), rexprGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"%", lexprGeQ.result, rexprGeQ.result, intermediate});
            return { quads, intermediate };
        }
}; // lexpr: AstNode, rexpr: AstNode

export class UnaryPlusExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> expr;

    public:
        UnaryPlusExpr(std::unique_ptr<AstNode> expr): AstNode("UnaryPlusExpr"), expr(std::move(expr)) {}
        ~UnaryPlusExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"+", valueGeQ.result, "", intermediate});
            return { quads, intermediate };
        }
}; // expr: AstNode

export class UnaryMinusExpr : public AstNode {
    private:
        std::unique_ptr<AstNode> expr;

    public:
        UnaryMinusExpr(std::unique_ptr<AstNode> expr): AstNode("UnaryMinusExpr"), expr(std::move(expr)) {}
        ~UnaryMinusExpr() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            const auto valueGeQ = expr->toQuadruples(globalLabelId, intermediateId + 1);
            quads.insert(quads.end(), valueGeQ.quads.begin(), valueGeQ.quads.end());
            const auto intermediate = getIntermediate(intermediateId);
            quads.emplace_back(Quadruple{"-", valueGeQ.result, "", intermediate});
            return { quads, intermediate };
        }
}; // expr: AstNode
export class FuncCall : public AstNode {
    private:
        Token id;
        std::vector<std::unique_ptr<AstNode>> exprs;

    public:
        FuncCall(Token id, std::vector<std::unique_ptr<AstNode>> arguments): AstNode("FuncCall"), id(id), exprs(std::move(arguments)) {}
        ~FuncCall() = default;

        GeQ toQuadruples(int& globalLabelId, int intermediateId) const override {
            Quadruples quads;
            for (int i = exprs.size() - 1; i >= 0; --i) {
                const auto argGeQ = exprs[i]->toQuadruples(globalLabelId, intermediateId + 1);
                quads.insert(quads.end(), argGeQ.quads.begin(), argGeQ.quads.end());
                quads.emplace_back(Quadruple{"PUSH", argGeQ.result, "", ""});
            }
            const auto intermediate = getIntermediate(intermediateId);
            return { quads, intermediate };
        }
}; // id: Token, exprs: AstNode[]
