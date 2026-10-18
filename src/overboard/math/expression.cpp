/**
 * @file   expression.cpp
 * @author Marvin Smith
 * @date   5/20/2026
 *
 * @brief  Implementation of the Expression class for handling mathematical expressions.
 */
#include <overboard/math/expression.hpp>

// C++ Standard Libraries
#include <cmath>
#include <functional>
#include <stdexcept>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>
#include <overboard/math/parser.hpp>
#include <overboard/math/ast/number_node.hpp>
#include <overboard/math/ast/binary_op_node.hpp>
#include <overboard/math/ast/function_node.hpp>
#include <overboard/math/ast/group_node.hpp>
#include <overboard/math/operators/function_operators.hpp>
#include <overboard/math/operators/unary_operators.hpp>

namespace ovb::math {

/********************************************/
/*              Constructor                 */
/********************************************/
Expression::Expression()
    : m_ast_root(std::make_unique<ast::Placeholder_Node>())
    , m_cursor_path() {
    // Empty expression initialized with placeholder for cursor position
}

/********************************************/
/*               Insert                     */
/********************************************/
void Expression::insert(core::Action_Code code) {
    using AC = core::Action_Code;

    // Map key codes to tree-based operations
    switch (code) {
        // Digits
        case AC::DIGIT_0:     insert_digit(0); break;
        case AC::DIGIT_1:     insert_digit(1); break;
        case AC::DIGIT_2:     insert_digit(2); break;
        case AC::DIGIT_3:     insert_digit(3); break;
        case AC::DIGIT_4:     insert_digit(4); break;
        case AC::DIGIT_5:     insert_digit(5); break;
        case AC::DIGIT_6:     insert_digit(6); break;
        case AC::DIGIT_7:     insert_digit(7); break;
        case AC::DIGIT_8:     insert_digit(8); break;
        case AC::DIGIT_9:     insert_digit(9); break;
        case AC::DECIMAL:     insert_digit(-1); break;

        // Hex digits
        case AC::HEX_A:       insert_digit(10); break;
        case AC::HEX_B:       insert_digit(11); break;
        case AC::HEX_C:       insert_digit(12); break;
        case AC::HEX_D:       insert_digit(13); break;
        case AC::HEX_E:       insert_digit(14); break;
        case AC::HEX_F:       insert_digit(15); break;

        // Operators
        case AC::ADD:         insert_operator(ast::Binary_Op::ADD); break;
        case AC::SUBTRACT:    insert_operator(ast::Binary_Op::SUBTRACT); break;
        case AC::MULTIPLY:    insert_operator(ast::Binary_Op::MULTIPLY); break;
        case AC::DIVIDE:      insert_operator(ast::Binary_Op::DIVIDE); break;
        case AC::POWER_N:     insert_operator(ast::Binary_Op::POWER); break;
        case AC::PERCENT:     insert_operator(ast::Binary_Op::MODULO); break;
        case AC::BIT_AND:     insert_operator(ast::Binary_Op::BIT_AND); break;
        case AC::BIT_OR:      insert_operator(ast::Binary_Op::BIT_OR); break;
        case AC::BIT_XOR:     insert_operator(ast::Binary_Op::BIT_XOR); break;
        case AC::BITSHIFT_LEFT:  insert_operator(ast::Binary_Op::SHIFT_LEFT); break;
        case AC::BITSHIFT_RIGHT: insert_operator(ast::Binary_Op::SHIFT_RIGHT); break;

        // Functions
        case AC::SIN:         insert_function(operators::Sin()); break;
        case AC::COS:         insert_function(operators::Cos()); break;
        case AC::TAN:         insert_function(operators::Tan()); break;
        case AC::ASIN:        insert_function(operators::Asin()); break;
        case AC::ACOS:        insert_function(operators::Acos()); break;
        case AC::ATAN:        insert_function(operators::Atan()); break;
        case AC::ATAN2:       insert_function(operators::Atan2()); break;
        case AC::COT:         insert_function(operators::Cot()); break;
        case AC::ACOT:        insert_function(operators::Acot()); break;
        case AC::SEC:         insert_function(operators::Sec()); break;
        case AC::CSC:         insert_function(operators::Csc()); break;
        case AC::SINH:        insert_function(operators::Sinh()); break;
        case AC::COSH:        insert_function(operators::Cosh()); break;
        case AC::TANH:        insert_function(operators::Tanh()); break;
        case AC::ASINH:       insert_function(operators::Asinh()); break;
        case AC::ACOSH:       insert_function(operators::Acosh()); break;
        case AC::ATANH:       insert_function(operators::Atanh()); break;
        case AC::COTH:        insert_function(operators::Coth()); break;
        case AC::SECH:        insert_function(operators::Sech()); break;
        case AC::CSCH:        insert_function(operators::Csch()); break;
        case AC::LOG:         insert_function(operators::Log()); break;
        case AC::LN:          insert_function(operators::Ln()); break;
        case AC::EXP:         insert_function(operators::Exp()); break;
        case AC::SQRT:        insert_function(operators::Sqrt()); break;
        case AC::CUBE_ROOT:   insert_function(operators::Cube_Root()); break;
        case AC::NTH_ROOT:    insert_function(operators::Nth_Root()); break;
        case AC::CEIL:        insert_function(operators::Ceil()); break;
        case AC::FLOOR:       insert_function(operators::Floor()); break;
        case AC::ABS:         insert_function(operators::Abs()); break;

        // Constants
        case AC::PI:          insert_function(operators::Pi()); break;
        case AC::EULER:       insert_function(operators::E()); break;
        case AC::PHI:         insert_function(operators::Phi()); break;
        case AC::TAU:         insert_function(operators::Tau()); break;

        // Special operators using factory pattern
        case AC::FACTORIAL:   insert_function(operators::Factorial()); break;
        case AC::RECIPROCAL:  insert_function(operators::Reciprocal()); break;
        case AC::POWER_2:     insert_function(operators::Power_2()); break;
        case AC::POWER_3:     insert_function(operators::Power_3()); break;

        // Parentheses - open/close grouping mode
        case AC::PAREN_OPEN:  insert_group(true);  break;
        case AC::PAREN_CLOSE: insert_group(false); break;

        // Approx function
        case AC::APPROX:      insert_function(operators::Approx()); break;

        // Matrix operations
        case AC::MAT_TRANSPOSE: insert_function(operators::Transpose()); break;
        case AC::MAT_DET:       insert_function(operators::Det()); break;
        case AC::MAT_INV:       insert_function(operators::Inv()); break;

        // Layer navigation handled in Calc_Engine, not here
        case AC::NEXT_LAYER:
        case AC::PREV_LAYER:
        case AC::GO_HOME_LAYER:
        case AC::GO_CONST_LAYER:
        case AC::GO_ALG_LAYER:
        case AC::GO_TRIG_LAYER:
        case AC::GO_VAR_LAYER:
        case AC::TOGGLE_MATH_LAYOUT:
            break;

        default:
#ifdef __cpp_exceptions
            throw std::invalid_argument("Expression::insert: unknown key code");
#endif
            break;
    }
}

/********************************************/
/*               Backspace                  */
/********************************************/
void Expression::backspace() {
    ast::Node* cursor_node = get_cursor_node();

    if (cursor_node == nullptr) {
        return;
    }

    // If cursor is on a number, remove the last digit
    if (cursor_node->kind() == ast::Node_Kind::NUMBER) {
        auto* number_node = static_cast<ast::Number_Node*>(cursor_node);
        double value = number_node->value();
        // Remove last digit by integer division
        if (value >= 10.0) {
            number_node->set_value(std::floor(value / 10.0));
        } else {
            // Single digit - replace with placeholder
            // Keep cursor path pointing to this location (don't reset to root)
            // This allows subsequent digit insertion to work inside functions
            auto new_node = std::make_unique<ast::Placeholder_Node>();
            replace_node_at_cursor(std::move(new_node));
        }
    }
    // If cursor is on a placeholder, check if parent is a function/constant/matrix to delete atomically
    else if (cursor_node->kind() == ast::Node_Kind::PLACEHOLDER) {
        if (!m_cursor_path.empty()) {

            // Check if parent is a function, constant, or matrix
            ast::Cursor_Path_Runtime parent_path = m_cursor_path.parent_path();
            ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);
            if (parent && (parent->kind() == ast::Node_Kind::FUNCTION || parent->kind() == ast::Node_Kind::CONSTANT)) {
                // Delete the parent function/constant atomically
                m_cursor_path = parent_path;
                auto new_node = std::make_unique<ast::Placeholder_Node>();
                replace_node_at_cursor(std::move(new_node));
                m_cursor_path = ast::Cursor_Path_Runtime();
                return;
            }

            // If parent is a matrix, replace current cell with placeholder (already is)
            // so delete the entire matrix
            if (parent && parent->kind() == ast::Node_Kind::MATRIX) {
                m_cursor_path = parent_path;
                auto new_node = std::make_unique<ast::Placeholder_Node>();
                replace_node_at_cursor(std::move(new_node));
                m_cursor_path = ast::Cursor_Path_Runtime();
                return;
            }

            // If parent is a binary op, delete the operator and move to left child
            if (parent && parent->kind() == ast::Node_Kind::BINARY_OP) {
                auto* bin_node = static_cast<ast::Binary_Op_Node*>(parent);
                // Replace with left child
                m_cursor_path = parent_path;
                auto new_node = bin_node->release_left();
                replace_node_at_cursor(std::move(new_node));
                // Cursor now points to left child at the parent's position
                // Don't reset cursor path - keep it pointing to this location
                return;
            }
        }
        // Already at empty state
    }
    // If cursor is on a binary op, delete the operator and move to left child
    else if (cursor_node->kind() == ast::Node_Kind::BINARY_OP) {
        auto* bin_node = static_cast<ast::Binary_Op_Node*>(cursor_node);
        // Replace with left child
        auto new_node = bin_node->release_left();
        replace_node_at_cursor(std::move(new_node));
        // Cursor now points to the left child (which replaced the binary op)
        // Don't modify cursor path - the replacement keeps us at the same position
    }
    // If cursor is on a function, delete the function atomically
    else if (cursor_node->kind() == ast::Node_Kind::FUNCTION) {
        auto new_node = std::make_unique<ast::Placeholder_Node>();
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path = ast::Cursor_Path_Runtime();
    }

    // If cursor is on a matrix, delete the matrix atomically
    else if (cursor_node->kind() == ast::Node_Kind::MATRIX) {
        auto new_node = std::make_unique<ast::Placeholder_Node>();
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path = ast::Cursor_Path_Runtime();
    }

    // If cursor is on a constant, delete the constant
    else if (cursor_node->kind() == ast::Node_Kind::CONSTANT) {
        auto new_node = std::make_unique<ast::Placeholder_Node>();
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path = ast::Cursor_Path_Runtime();
    }
}

/********************************************/
/*               Cursor Left                */
/********************************************/
void Expression::cursor_left() {
    // Tree-based cursor left navigation
    if (m_cursor_path.empty()) {
        // At root, can't go left
        return;
    }

    // Get current node
    ast::Node* current = get_cursor_node();
    if (!current) {
        return;
    }

    // Pop one level: if there's a previous sibling, land on it.
    // If we're at index 0, land on the parent itself (making it a cursor stop).
    ast::Cursor_Path_Runtime path = m_cursor_path;
    std::size_t idx = path[path.depth() - 1];
    path.pop();
    if (idx > 0) {
        // Land on the previous sibling (may be internal or leaf)
        path.push(idx - 1);
    }
    // else: land on the parent (path now points to the parent node)
    m_cursor_path = path;
}

/********************************************/
/*               Cursor Right               */
/********************************************/
void Expression::cursor_right() {
    // Tree-based cursor right navigation
    if (m_cursor_path.empty()) {
        // At root, try to move to first child
        if (m_ast_root && m_ast_root->child_count() > 0) {
            m_cursor_path.push(0);
        }
        return;
    }

    // Get current node
    ast::Node* current = get_cursor_node();
    if (!current) {
        return;
    }

    // If at a leaf node, try to move to next sibling; if none, climb up until we
    // find a level with a next sibling (allows exiting a function/group).
    if (current->child_count() == 0) {
        ast::Cursor_Path_Runtime path = m_cursor_path;
        while (!path.empty()) {
            std::size_t idx = path[path.depth() - 1];
            path.pop();
            ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), path);
            if (parent && idx + 1 < parent->child_count()) {
                path.push(idx + 1);
                m_cursor_path = path;
                return;
            }
            // No next sibling at this level — keep climbing
        }
        // Reached the root with no next sibling — move to root
        m_cursor_path = path; // now empty = root
    } else {
        // At internal node, move to first child
        m_cursor_path.push(0);
    }
}

/********************************************/
/*               Cursor Up                  */
/********************************************/
void Expression::cursor_up() {
    if (m_cursor_path.empty()) return;

    // Walk up to find the nearest matrix ancestor
    ast::Cursor_Path_Runtime path = m_cursor_path;
    while (!path.empty()) {
        std::size_t idx = path[path.depth() - 1];
        ast::Cursor_Path_Runtime parent_path = path;
        parent_path.pop();
        const ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);

        if (parent && parent->kind() == ast::Node_Kind::MATRIX) {
            const auto* mat = static_cast<const ast::Matrix_Node*>(parent);
            int cols = mat->cols();
            int current_flat = static_cast<int>(idx);
            int current_row = current_flat / cols;
            int current_col = current_flat % cols;
            if (current_row > 0) {
                int new_flat = (current_row - 1) * cols + current_col;
                path.path()[path.depth() - 1] = static_cast<std::size_t>(new_flat);
                m_cursor_path = path;
            }
            return;
        }
        path = parent_path;
    }
}

/********************************************/
/*              Cursor Down                 */
/********************************************/
void Expression::cursor_down() {
    if (m_cursor_path.empty()) return;

    // Walk up to find the nearest matrix ancestor
    ast::Cursor_Path_Runtime path = m_cursor_path;

    while (!path.empty()) {
        std::size_t idx = path[path.depth() - 1];
        ast::Cursor_Path_Runtime parent_path = path;
        parent_path.pop();
        const ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);
        if (parent && parent->kind() == ast::Node_Kind::MATRIX) {
            const auto* mat = static_cast<const ast::Matrix_Node*>(parent);
            int rows = mat->rows();
            int cols = mat->cols();
            int current_flat = static_cast<int>(idx);
            int current_row = current_flat / cols;
            int current_col = current_flat % cols;
            if (current_row < rows - 1) {
                int new_flat = (current_row + 1) * cols + current_col;
                path.path()[path.depth() - 1] = static_cast<std::size_t>(new_flat);
                m_cursor_path = path;
            }
            return;
        }
        path = parent_path;
    }
}

/********************************************/
/*            Insert Node                   */
/********************************************/
void Expression::insert_node( const ast::Node& node ) {
    ast::Node* cursor_node = get_cursor_node();
    if (!cursor_node) {
        return;
    }

    if (cursor_node->kind() == ast::Node_Kind::PLACEHOLDER) {
        replace_node_at_cursor(node.clone());
        m_decimal_position = 0;

        // If the new node has children (e.g. Matrix_Node), descend to first child
        ast::Node* new_cursor = get_cursor_node();
        if (new_cursor && new_cursor->child_count() > 0) {
            m_cursor_path.push(0);
        }
    }
}

/********************************************/
/*               Clear                      */
/********************************************/
void Expression::clear() {
    m_ast_root = std::make_unique<ast::Placeholder_Node>();
    m_cursor_path = ast::Cursor_Path_Runtime();
    m_decimal_position = 0;
    m_group_depth = 0;
}

/********************************************/
/*            Set Number                    */
/********************************************/
void Expression::set_number(double value) {
    m_ast_root = std::make_unique<ast::Number_Node>(value);
    m_cursor_path = ast::Cursor_Path_Runtime();  // Cursor at root (the number)
    m_decimal_position = 0;
    m_group_depth = 0;
}

/********************************/
/*        Has Placeholder       */
/********************************/
bool Expression::has_placeholder() const {
    // Tree-based traversal to check for placeholder nodes
    if (!m_ast_root) {
        return false;
    }

    std::function<bool(const ast::Node*)> check_placeholder;
    check_placeholder = [&check_placeholder](const ast::Node* node) -> bool {
        if (!node) {
            return false;
        }
        if (node->kind() == ast::Node_Kind::PLACEHOLDER) {
            return true;
        }
        // Check children recursively
        for (std::size_t i = 0; i < static_cast<std::size_t>(node->child_count()); ++i) {
            if (check_placeholder(node->child_at(i))) {
                return true;
            }
        }
        return false;
    };

    return check_placeholder(m_ast_root.get());
}

/**********************************************/
/*     Remove Trailing Placeholder            */
/**********************************************/
void Expression::remove_trailing_placeholder() {
    // Tree-based removal of trailing placeholder
    // For now, this is a no-op since the tree doesn't have a concept of "trailing"
    // This will be handled properly when we implement full tree manipulation
}

/*********************************/
/*         Delete Right          */
/*********************************/
void Expression::delete_right() {
    // Find the node to the right of cursor and replace with placeholder
    ast::Cursor_Path_Runtime right_path = ast::cursor_right(m_ast_root.get(), m_cursor_path);
    if (right_path == m_cursor_path) {
        // Can't move right, nothing to delete
        return;
    }

    // Get the node at the right position without moving cursor
    ast::Node* node_to_delete = ast::get_node_at_path(m_ast_root.get(), right_path);
    if (!node_to_delete) {
        return;
    }

    // We need to replace this node with a placeholder
    // This is tricky because we need to find its parent and replace it
    if (right_path.empty()) {
        // It's the root - replace root
        m_ast_root = std::make_unique<ast::Placeholder_Node>();
    } else {
        // Find parent and replace the appropriate child
        ast::Cursor_Path_Runtime parent_path = right_path.parent_path();
        ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);
        if (!parent) {
            return;
        }

        size_t child_index = right_path[right_path.depth() - 1];

        // Handle different parent types
        if (auto* bin_op = dynamic_cast<ast::Binary_Op_Node*>(parent)) {
            if (child_index == 0) {
                bin_op->set_left(std::make_unique<ast::Placeholder_Node>());
            } else {
                bin_op->set_right(std::make_unique<ast::Placeholder_Node>());
            }
        } else if (auto* func = dynamic_cast<ast::Function_Node*>(parent)) {
            func->set_child(child_index, std::make_unique<ast::Placeholder_Node>());
        } else if (auto* group = dynamic_cast<ast::Group_Node*>(parent)) {
            group->set_child(std::make_unique<ast::Placeholder_Node>());
        } else if (auto* unary = dynamic_cast<ast::Unary_Op_Node*>(parent)) {
            unary->set_operand(std::make_unique<ast::Placeholder_Node>());
        } else if (auto* fact = dynamic_cast<ast::Factorial_Node*>(parent)) {
            fact->set_operand(std::make_unique<ast::Placeholder_Node>());
        }
    }
}

/*********************************/
/*        Evaluate String        */
/*********************************/
std::string Expression::eval_string() const {
    // Tree-based traversal to build evaluation string
    if (!m_ast_root) {
        return "";
    }

    // If root is a placeholder, return empty string (empty expression)
    if (m_ast_root->kind() == ast::Node_Kind::PLACEHOLDER) {
        return "";
    }

    Node_Visitor_String to_eval_string;
    to_eval_string = [this, &to_eval_string](const ast::Node* node) -> std::string {
        if (!node) {
            return "";
        }

        switch (node->kind()) {
            case ast::Node_Kind::PLACEHOLDER:
                return "0";  // Makes expression parsable: "8+0" is valid
            case ast::Node_Kind::NUMBER: {
                const auto* num = static_cast<const ast::Number_Node*>(node);
                std::string str = num->to_string();
                // If cursor is on this number and in decimal mode with no decimal digits, append decimal point
                if (m_cursor_path.empty() && node == m_ast_root.get() && m_decimal_position > 0 && str.find('.') == std::string::npos) {
                    str += ".";
                }
                return str;
            }
            case ast::Node_Kind::BINARY_OP: {
                const auto* bin = static_cast<const ast::Binary_Op_Node*>(node);
                std::string left = to_eval_string(bin->left().get());
                std::string right = to_eval_string(bin->right().get());
                std::string op_str;
                switch (bin->op()) {
                    case ast::Binary_Op::ADD:       op_str = "+"; break;
                    case ast::Binary_Op::SUBTRACT:  op_str = "-"; break;
                    case ast::Binary_Op::MULTIPLY:  op_str = "*"; break;
                    case ast::Binary_Op::DIVIDE:    op_str = "/"; break;
                    case ast::Binary_Op::POWER:     op_str = "^"; break;
                    case ast::Binary_Op::MODULO:    op_str = "%"; break;
                    case ast::Binary_Op::BIT_AND:   op_str = "&"; break;
                    case ast::Binary_Op::BIT_OR:    op_str = "|"; break;
                    case ast::Binary_Op::BIT_XOR:   op_str = "^"; break;
                    case ast::Binary_Op::SHIFT_LEFT: op_str = "<<"; break;
                    case ast::Binary_Op::SHIFT_RIGHT:op_str = ">>"; break;
                }
                return "(" + left + op_str + right + ")";
            }
            case ast::Node_Kind::FUNCTION: {
                const auto* func = static_cast<const ast::Function_Node*>(node);
                std::string args;
                for (std::size_t i = 0; i < static_cast<std::size_t>(func->child_count()); ++i) {
                    if (i > 0) args += ",";
                    args += to_eval_string(func->child_at(i));
                }
                return func->name() + "(" + args + ")";
            }
            case ast::Node_Kind::FACTORIAL: {
                const auto* fact = static_cast<const ast::Factorial_Node*>(node);
                return to_eval_string(fact->child_at(0)) + "!";
            }
            case ast::Node_Kind::UNARY_OP: {
                const auto* unary = static_cast<const ast::Unary_Op_Node*>(node);
                std::string operand = to_eval_string(unary->operand().get());
                std::string op_str;
                switch (unary->op()) {
                    case ast::Unary_Op::NEGATE:  op_str = "-"; break;
                    case ast::Unary_Op::BIT_NOT: op_str = "~"; break;
                    case ast::Unary_Op::PERCENT: op_str = "%"; break;
                }
                return op_str + operand;
            }
            case ast::Node_Kind::GROUP: {
                const auto* group = static_cast<const ast::Group_Node*>(node);
                return "(" + to_eval_string(group->child().get()) + ")";
            }
            case ast::Node_Kind::MATRIX: {
                const auto* mat = static_cast<const ast::Matrix_Node*>(node);
                std::string s = "[";
                for (int r = 0; r < mat->rows(); ++r) {
                    if (r > 0) s += ";";
                    s += "[";
                    for (int c = 0; c < mat->cols(); ++c) {
                        if (c > 0) s += ",";
                        s += to_eval_string(mat->child_at(static_cast<std::size_t>(r * mat->cols() + c)));
                    }
                    s += "]";
                }
                return s + "]";
            }
            case ast::Node_Kind::CONSTANT: {
                const auto* const_node = static_cast<const ast::Constant_Node*>(node);
                switch (const_node->id()) {
                    case ast::Constant_Id::PI:    return "pi";
                    case ast::Constant_Id::E:     return "e";
                    case ast::Constant_Id::PHI:   return "phi";
                    case ast::Constant_Id::TAU:   return "tau";
                    default: return "";
                }
            }
            default:
                return "";
        }
    };

    return to_eval_string(m_ast_root.get());
}

/**********************************************/
/*          Render a String                   */
/**********************************************/
std::string Expression::render_string() const {
    // Tree-based traversal to build display string
    if (!m_ast_root) {
        return "";
    }

    Node_Visitor_String to_render_string;
    to_render_string = [&to_render_string](const ast::Node* node) -> std::string {
        if (!node) {
            return "";
        }

        switch (node->kind()) {
            case ast::Node_Kind::PLACEHOLDER:
                return "□";   // Unicode white square for display
            case ast::Node_Kind::NUMBER: {
                const auto* num = static_cast<const ast::Number_Node*>(node);
                return num->to_string();
            }
            case ast::Node_Kind::BINARY_OP: {
                const auto* bin = static_cast<const ast::Binary_Op_Node*>(node);
                std::string left = to_render_string(bin->left().get());
                std::string right = to_render_string(bin->right().get());
                std::string op_str;
                switch (bin->op()) {
                    case ast::Binary_Op::ADD:       op_str = "+"; break;
                    case ast::Binary_Op::SUBTRACT:  op_str = "-"; break;
                    case ast::Binary_Op::MULTIPLY:  op_str = "×"; break;
                    case ast::Binary_Op::DIVIDE:    op_str = "÷"; break;
                    case ast::Binary_Op::POWER:     op_str = "^"; break;
                    case ast::Binary_Op::MODULO:    op_str = "%"; break;
                    case ast::Binary_Op::BIT_AND:   op_str = "&"; break;
                    case ast::Binary_Op::BIT_OR:    op_str = "|"; break;
                    case ast::Binary_Op::BIT_XOR:   op_str = "^"; break;
                    case ast::Binary_Op::SHIFT_LEFT: op_str = "<<"; break;
                    case ast::Binary_Op::SHIFT_RIGHT:op_str = ">>"; break;
                }
                return left + op_str + right;
            }
            case ast::Node_Kind::FUNCTION: {
                const auto* func = static_cast<const ast::Function_Node*>(node);
                std::string args;
                for (std::size_t i = 0; i < static_cast<std::size_t>(func->child_count()); ++i) {
                    if (i > 0) args += ",";
                    args += to_render_string(func->child_at(i));
                }
                return func->name() + "(" + args + ")";
            }
            case ast::Node_Kind::FACTORIAL: {
                const auto* fact = static_cast<const ast::Factorial_Node*>(node);
                return to_render_string(fact->child_at(0)) + "!";
            }
            case ast::Node_Kind::UNARY_OP: {
                const auto* unary = static_cast<const ast::Unary_Op_Node*>(node);
                std::string operand = to_render_string(unary->operand().get());
                std::string op_str;
                switch (unary->op()) {
                    case ast::Unary_Op::NEGATE:  op_str = "-"; break;
                    case ast::Unary_Op::BIT_NOT: op_str = "~"; break;
                    case ast::Unary_Op::PERCENT: op_str = "%"; break;
                }
                return op_str + operand;
            }
            case ast::Node_Kind::GROUP: {
                const auto* group = static_cast<const ast::Group_Node*>(node);
                return "(" + to_render_string(group->child().get()) + ")";
            }
            case ast::Node_Kind::MATRIX: {
                const auto* mat = static_cast<const ast::Matrix_Node*>(node);
                std::string s = "[";
                for (int r = 0; r < mat->rows(); ++r) {
                    if (r > 0) s += "; ";
                    s += "[";
                    for (int c = 0; c < mat->cols(); ++c) {
                        if (c > 0) s += ", ";
                        s += to_render_string(mat->child_at(static_cast<std::size_t>(r * mat->cols() + c)));
                    }
                    s += "]";
                }
                return s + "]";
            }
            case ast::Node_Kind::CONSTANT: {
                const auto* const_node = static_cast<const ast::Constant_Node*>(node);
                switch (const_node->id()) {
                    case ast::Constant_Id::PI:    return "pi";
                    case ast::Constant_Id::E:     return "e";
                    case ast::Constant_Id::PHI:   return "phi";
                    case ast::Constant_Id::TAU:   return "tau";
                    default: return "";
                }
            }
            default:
                return "";
        }
    };

    return to_render_string(m_ast_root.get());
}

/**********************************************/
/*          Cursor Glyph Position             */
/**********************************************/
std::size_t Expression::cursor_glyph_pos() const {
    // Tree-based cursor position calculation
    // For now, this is a simplified implementation that counts glyphs up to the cursor node
    // Full implementation will need to handle cursor positions between nodes
    if (!m_ast_root) {
        return 0;
    }

    // If cursor is at root (empty path), count all glyphs in the tree
    if (m_cursor_path.empty()) {
        // If root is a placeholder (empty expression), cursor is at position 0
        if (m_ast_root->kind() == ast::Node_Kind::PLACEHOLDER) {
            return 0;
        }
        return render_string().length();
    }

    std::function<std::size_t(const ast::Node*, const ast::Cursor_Path_Runtime&, int)> count_glyphs_to_cursor;
    count_glyphs_to_cursor = [&count_glyphs_to_cursor](const ast::Node* node, const ast::Cursor_Path_Runtime& path, int depth) -> std::size_t {
        if (!node) {
            return 0;
        }

        // If we've reached the cursor depth, return 0 (cursor is at this position)
        if (depth >= static_cast<int>(path.depth())) {
            return 0;
        }

        // Count glyphs for this node's representation
        std::size_t glyph_count = 0;
        switch (node->kind()) {
            case ast::Node_Kind::PLACEHOLDER:
                glyph_count = 1;  // "□"
                break;
            case ast::Node_Kind::NUMBER: {
                const auto* num = static_cast<const ast::Number_Node*>(node);
                std::string num_str = std::to_string(static_cast<long long>(num->value()));
                glyph_count = num_str.length();
                break;
            }
            case ast::Node_Kind::BINARY_OP: {
                const auto* bin = static_cast<const ast::Binary_Op_Node*>(node);
                std::string op_str;
                switch (bin->op()) {
                    case ast::Binary_Op::ADD:       op_str = "+"; break;
                    case ast::Binary_Op::SUBTRACT:  op_str = "-"; break;
                    case ast::Binary_Op::MULTIPLY:  op_str = "×"; break;
                    case ast::Binary_Op::DIVIDE:    op_str = "÷"; break;
                    case ast::Binary_Op::POWER:     op_str = "^"; break;
                    case ast::Binary_Op::MODULO:    op_str = "%"; break;
                    case ast::Binary_Op::BIT_AND:   op_str = "&"; break;
                    case ast::Binary_Op::BIT_OR:    op_str = "|"; break;
                    case ast::Binary_Op::BIT_XOR:   op_str = "^"; break;
                    case ast::Binary_Op::SHIFT_LEFT: op_str = "<<"; break;
                    case ast::Binary_Op::SHIFT_RIGHT:op_str = ">>"; break;
                }
                glyph_count = op_str.length();
                break;
            }
            case ast::Node_Kind::FUNCTION: {
                const auto* func = static_cast<const ast::Function_Node*>(node);
                glyph_count = func->name().length() + 2;  // name + "()"
                break;
            }
            default:
                break;
        }

        // If cursor is at this depth, return the glyph count
        if (depth == static_cast<int>(path.depth()) - 1) {
            return glyph_count;
        }

        // Otherwise, traverse to the child at the cursor path index
        if (depth < static_cast<int>(path.depth())) {
            std::size_t child_index = path.path()[static_cast<std::size_t>(depth)];
            std::size_t child_glyphs = 0;
            for (std::size_t i = 0; i < static_cast<std::size_t>(node->child_count()); ++i) {
                if (i == child_index) {
                    child_glyphs = count_glyphs_to_cursor(node->child_at(i), path, depth + 1);
                    break;
                }
                // Count glyphs for siblings before the cursor
                // This is a simplified approach - full implementation needs proper traversal
            }
            return glyph_count + child_glyphs;
        }

        return glyph_count;
    };

    return count_glyphs_to_cursor(m_ast_root.get(), m_cursor_path, 0);
}

/********************************************/
/*           Get Cursor Node                 */
/********************************************/
ast::Node* Expression::get_cursor_node() {
    return ast::get_node_at_path(m_ast_root.get(), m_cursor_path);
}

const ast::Node* Expression::get_cursor_node() const {
    return ast::get_node_at_path(m_ast_root.get(), m_cursor_path);
}

/********************************************/
/*         Replace Node At Cursor            */
/********************************************/
void Expression::replace_node_at_cursor( ast::Node::ptr_t new_node ) {
    if (m_cursor_path.empty()) {
        // Replace root
        m_ast_root = std::move(new_node);
        return;
    }

    // Navigate to parent node
    ast::Cursor_Path_Runtime parent_path = m_cursor_path.parent_path();
    ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);

    if (parent == nullptr) {
        return;
    }

    // Get the index of the child to replace
    size_t child_index = m_cursor_path[m_cursor_path.depth() - 1];

    // Replace the child based on node type
    if (parent->kind() == ast::Node_Kind::BINARY_OP) {
        auto* bin_node = static_cast<ast::Binary_Op_Node*>(parent);
        if (child_index == 0) {
            bin_node->set_left(std::move(new_node));
        } else if (child_index == 1) {
            bin_node->set_right(std::move(new_node));
        }
    } else if (parent->kind() == ast::Node_Kind::FUNCTION) {
        auto* func_node = static_cast<ast::Function_Node*>(parent);
        func_node->set_child(child_index, std::move(new_node));
    } else if (parent->kind() == ast::Node_Kind::GROUP) {
        auto* group_node = static_cast<ast::Group_Node*>(parent);
        if (child_index == 0) {
            group_node->set_child(std::move(new_node));
        }
    } else if (parent->kind() == ast::Node_Kind::UNARY_OP) {
        auto* unary_node = static_cast<ast::Unary_Op_Node*>(parent);
        if (child_index == 0) {
            unary_node->set_operand(std::move(new_node));
        }
    } else if (parent->kind() == ast::Node_Kind::FACTORIAL) {
        auto* fact_node = static_cast<ast::Factorial_Node*>(parent);
        fact_node->set_operand(std::move(new_node));
    } else if (parent->kind() == ast::Node_Kind::MATRIX) {
        auto* mat_node = static_cast<ast::Matrix_Node*>(parent);
        mat_node->set_child(child_index, std::move(new_node));
    }
}

/********************************************/
/*           Insert Digit                   */
/********************************************/
void Expression::insert_digit( int digit ) {
    ast::Node* cursor_node = get_cursor_node();

    if (cursor_node == nullptr) {
        return;
    }

    // If cursor is on a placeholder, replace it with a number
    if (cursor_node->kind() == ast::Node_Kind::PLACEHOLDER) {
        if (digit == -1) {
            // Decimal point alone becomes 0.
            auto new_node = std::make_unique<ast::Number_Node>(0.0);
            replace_node_at_cursor(std::move(new_node));
            m_decimal_position = 1; // Enter decimal mode
        } else if (digit >= 10) {
            // Hex digit A-F (10-15), convert to hex value
            double value = static_cast<double>(digit);
            auto new_node = std::make_unique<ast::Number_Node>(value);
            replace_node_at_cursor(std::move(new_node));
            m_decimal_position = 0;
        } else {
            double value = static_cast<double>(digit);
            auto new_node = std::make_unique<ast::Number_Node>(value);
            replace_node_at_cursor(std::move(new_node));
            m_decimal_position = 0;
        }
        // Cursor stays at the same position (no path change needed)
    }
    // If cursor is on a number, append the digit
    else if (cursor_node->kind() == ast::Node_Kind::NUMBER) {
        auto* number_node = static_cast<ast::Number_Node*>(cursor_node);
        double current_value = number_node->value();

        if (digit == -1) {
            // Decimal point - only allow if not already in decimal mode
            if (m_decimal_position == 0) {
                m_decimal_position = 1; // Enter decimal mode
            }
            // If already has decimal, ignore
            // Note: cursor stays on the number node for subsequent digit input
            // Cursor path should remain empty (pointing to root number)
        } else if (digit >= 10) {
            // Hex digit A-F - append as hex digit
            // For simplicity, treat as decimal append for now
            // Full implementation would need hex mode tracking
            double new_value = current_value * 16.0 + static_cast<double>(digit);
            number_node->set_value(new_value);
        } else {
            // Decimal digit
            if (m_decimal_position > 0) {
                // In decimal mode - add digit at current decimal position
                double divisor = std::pow(10.0, m_decimal_position);
                double new_value = current_value + (static_cast<double>(digit) / divisor);
                number_node->set_value(new_value);
                m_decimal_position++;
            } else {
                // Integer mode - append digit
                double new_value = current_value * 10.0 + static_cast<double>(digit);
                number_node->set_value(new_value);
            }
        }
    }
    // If cursor is on a placeholder, replace it with a number
    else if (cursor_node->kind() == ast::Node_Kind::PLACEHOLDER) {
        if (digit == -1) {
            // Decimal point alone becomes 0.
            auto new_node = std::make_unique<ast::Number_Node>(0.0);
            replace_node_at_cursor(std::move(new_node));
            m_decimal_position = 1; // Enter decimal mode
        } else if (digit >= 10) {
            // Hex digit A-F (10-15), convert to hex value
            double value = static_cast<double>(digit);
            auto new_node = std::make_unique<ast::Number_Node>(value);
            replace_node_at_cursor(std::move(new_node));
            m_decimal_position = 0;
        } else {
            double value = static_cast<double>(digit);
            auto new_node = std::make_unique<ast::Number_Node>(value);
            replace_node_at_cursor(std::move(new_node));
            m_decimal_position = 0;
        }
        m_cursor_path = ast::Cursor_Path_Runtime();
    }
    // If cursor is on anything else, ignore (operators, functions, constants)
    else {
        // Do nothing - digits can only be inserted into numbers or placeholders
    }
}

/********************************************/
/*         Insert Operator                  */
/********************************************/
void Expression::insert_operator( ast::Binary_Op op ) {

    ast::Node* cursor_node = get_cursor_node();

    if (cursor_node == nullptr) {
        return;
    }

    // Reset decimal position when inserting operator
    m_decimal_position = 0;

    // If cursor is on a placeholder, replace with binary op with placeholder children
    if (cursor_node->kind() == ast::Node_Kind::PLACEHOLDER) {
        auto left = std::make_unique<ast::Placeholder_Node>();
        auto right = std::make_unique<ast::Placeholder_Node>();
        auto new_node = std::make_unique<ast::Binary_Op_Node>(op, std::move(left), std::move(right));
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path.push(0); // Move cursor to left placeholder
    }
    // If cursor is on a number, create binary op with number as left child
    else if (cursor_node->kind() == ast::Node_Kind::NUMBER) {
        auto* number_node = static_cast<ast::Number_Node*>(cursor_node);
        double value = number_node->value();
        auto left = std::make_unique<ast::Number_Node>(value);
        auto right = std::make_unique<ast::Placeholder_Node>();
        auto new_node = std::make_unique<ast::Binary_Op_Node>(op, std::move(left), std::move(right));
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path.push(1); // Move cursor to right placeholder
    }
    // If cursor is on a binary op, handle precedence by restructuring
    else if (cursor_node->kind() == ast::Node_Kind::BINARY_OP) {
        auto* bin_node = static_cast<ast::Binary_Op_Node*>(cursor_node);
        ast::Binary_Op existing_op = bin_node->op();

        // Simple precedence: if new op has higher or equal precedence, make it the new root
        // This is a simplified approach - full implementation needs proper precedence table
        bool new_op_higher = (op == ast::Binary_Op::MULTIPLY || op == ast::Binary_Op::DIVIDE) &&
                            (existing_op == ast::Binary_Op::ADD || existing_op == ast::Binary_Op::SUBTRACT);

        if (new_op_higher) {
            // New op becomes parent of existing tree
            auto left = std::move(m_ast_root);
            auto right = std::make_unique<ast::Placeholder_Node>();
            m_ast_root = std::make_unique<ast::Binary_Op_Node>(op, std::move(left), std::move(right));
            m_cursor_path = ast::Cursor_Path_Runtime({1});
        } else {
            // New op becomes right child of existing op
            auto right = std::make_unique<ast::Placeholder_Node>();
            // Replace right child with new binary op
            auto new_right = std::make_unique<ast::Binary_Op_Node>(op, bin_node->release_right(), std::move(right));
            bin_node->set_right(std::move(new_right));
            m_cursor_path.push(1);
            m_cursor_path.push(1);
        }
    }
    // If cursor is on a function, make the function the left operand
    else if (cursor_node->kind() == ast::Node_Kind::FUNCTION) {
        // Clone the function to use as left operand
        auto left = cursor_node->clone();
        auto right = std::make_unique<ast::Placeholder_Node>();
        auto new_node = std::make_unique<ast::Binary_Op_Node>(op, std::move(left), std::move(right));
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path.push(1);
    }
    // If cursor is on a constant, make the constant the left operand
    else if (cursor_node->kind() == ast::Node_Kind::CONSTANT) {
        auto* const_node = static_cast<ast::Constant_Node*>(cursor_node);
        auto left = std::make_unique<ast::Constant_Node>(const_node->id());
        auto right = std::make_unique<ast::Placeholder_Node>();
        auto new_node = std::make_unique<ast::Binary_Op_Node>(op, std::move(left), std::move(right));
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path.push(1);
    }
    // If cursor is on a group, make the group the left operand
    else if (cursor_node->kind() == ast::Node_Kind::GROUP) {
        auto left = cursor_node->clone();
        auto right = std::make_unique<ast::Placeholder_Node>();
        auto new_node = std::make_unique<ast::Binary_Op_Node>(op, std::move(left), std::move(right));
        replace_node_at_cursor(std::move(new_node));
        m_cursor_path.push(1);
    }
}

/********************************************/
/*         Insert Function                  */
/********************************************/
void Expression::insert_function( const operators::I_Operator& func ) {

    ast::Node* cursor_node = get_cursor_node();

    if (cursor_node == nullptr) {
        return;
    }

    // Reset decimal position when inserting function
    m_decimal_position = 0;

    // If cursor is on a placeholder, replace with function with placeholder argument(s)
    if (cursor_node->kind() == ast::Node_Kind::PLACEHOLDER) {
        std::vector<ast::Node::ptr_t> args;
        for (int i = 0; i < func.operand_count(); ++i) {
            args.push_back(std::make_unique<ast::Placeholder_Node>());
        }
        auto new_node = func.create_node(std::move(args));
        replace_node_at_cursor(std::move(new_node));
        if (func.operand_count() > 0) {
            m_cursor_path.push(0); // Move cursor to first argument placeholder
        }
        // For constants (0 operands), cursor stays at root
    }
    // If cursor is on a number, create function with number as first argument
    else if (cursor_node->kind() == ast::Node_Kind::NUMBER) {
        auto* number_node = static_cast<ast::Number_Node*>(cursor_node);
        double value = number_node->value();
        std::vector<ast::Node::ptr_t> args;
        args.push_back(std::make_unique<ast::Number_Node>(value));
        // For multi-argument functions, add placeholders for remaining arguments
        for (int i = 1; i < func.operand_count(); ++i) {
            args.push_back(std::make_unique<ast::Placeholder_Node>());
        }
        auto new_node = func.create_node(std::move(args));
        replace_node_at_cursor(std::move(new_node));
        // Move cursor to second argument if multi-arg, otherwise stay on first argument
        if (func.operand_count() > 1) {
            m_cursor_path.push(1); // Move cursor to second argument placeholder
        } else {
            m_cursor_path.push(0); // Move cursor to first argument
        }
    }
}

/********************************************/
/*           Insert Group (Parens)            */
/********************************************/
void Expression::insert_group( bool open ) {

    ast::Node* cursor_node = get_cursor_node();

    if (cursor_node == nullptr) {
        return;
    }

    // Reset decimal position when entering/exiting group
    m_decimal_position = 0;

    if (open) {
        // Opening a group: wrap current node in a Group_Node
        // If cursor is on a placeholder, wrap it
        // If cursor is on an existing expression, wrap the whole expression

        ast::Node::ptr_t child_to_wrap;

        if (m_cursor_path.empty()) {
            // At root - wrap the entire expression
            child_to_wrap = std::move(m_ast_root);
        } else {
            // Inside a node - need to get parent and replace child
            ast::Cursor_Path_Runtime parent_path = m_cursor_path.parent_path();
            ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);

            if (parent == nullptr) {
                return;
            }

            // Get current index
            size_t child_index = m_cursor_path[m_cursor_path.depth() - 1];

            // Get the child to wrap
            if (auto* bin_op = dynamic_cast<ast::Binary_Op_Node*>(parent)) {
                if (child_index == 0) {
                    child_to_wrap = bin_op->release_left();
                } else {
                    child_to_wrap = bin_op->release_right();
                }
            } else if (auto* func = dynamic_cast<ast::Function_Node*>(parent)) {
                child_to_wrap = func->release_child(child_index);
            } else if (auto* group = dynamic_cast<ast::Group_Node*>(parent)) {
                child_to_wrap = group->release_child();
            } else if (auto* unary = dynamic_cast<ast::Unary_Op_Node*>(parent)) {
                child_to_wrap = unary->release_operand();
            } else if (auto* fact = dynamic_cast<ast::Factorial_Node*>(parent)) {
                child_to_wrap = fact->release_operand();
            } else {
                // Unknown parent type, can't wrap
                return;
            }
        }

        // Create the group node with the child
        auto group_node = std::make_unique<ast::Group_Node>(std::move(child_to_wrap));

        // Place the group node
        if (m_cursor_path.empty()) {
            m_ast_root = std::move(group_node);
        } else {
            // Replace the child with the group node at cursor path
            // We need to navigate to parent and replace
            ast::Cursor_Path_Runtime parent_path = m_cursor_path.parent_path();
            ast::Node* parent = ast::get_node_at_path(m_ast_root.get(), parent_path);

            if (parent != nullptr) {
                size_t child_index = m_cursor_path[m_cursor_path.depth() - 1];

                if (auto* bin_op = dynamic_cast<ast::Binary_Op_Node*>(parent)) {
                    if (child_index == 0) {
                        bin_op->set_left(std::move(group_node));
                    } else {
                        bin_op->set_right(std::move(group_node));
                    }
                } else if (auto* func = dynamic_cast<ast::Function_Node*>(parent)) {
                    func->set_child(child_index, std::move(group_node));
                } else if (auto* group = dynamic_cast<ast::Group_Node*>(parent)) {
                    group->set_child(std::move(group_node));
                } else if (auto* unary = dynamic_cast<ast::Unary_Op_Node*>(parent)) {
                    unary->set_operand(std::move(group_node));
                } else if (auto* fact = dynamic_cast<ast::Factorial_Node*>(parent)) {
                    fact->set_operand(std::move(group_node));
                }
            }
        }

        // Move cursor into the group (to the child)
        m_cursor_path.push(0);
        m_group_depth++;

    } else {
        // Closing a group: move cursor up and out of the group
        if (m_group_depth > 0) {
            // Check if we're inside a group by looking at the path
            // Navigate up to find the group node
            if (!m_cursor_path.empty()) {
                // Pop to parent level
                m_cursor_path.pop();
                m_group_depth--;
            }
        }
        // If not in a group, ignore the close paren
    }
}

} // namespace ovb::math
