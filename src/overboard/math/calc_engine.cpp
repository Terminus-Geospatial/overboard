/**
 * @file   calc_engine.cpp
 * @author Marvin Smith
 * @date   5/20/2026
 *
 * @brief  Implementation of the Calc_Engine class for handling calculator operations.
 */
#include <overboard/math/calc_engine.hpp>

// C++ Standard Libraries
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>
#include <overboard/math/ast/function_node.hpp>
#include <overboard/math/ast/matrix_node.hpp>
#include <overboard/math/ast/number_node.hpp>
#include <overboard/math/ast/placeholder_node.hpp>
#include <overboard/math/parser.hpp>

namespace ovb::math {

/****************************/
/*     Constructor/Reset    */
/****************************/
Calc_Engine::Calc_Engine() {
    reset();
}

/****************************/
/*     Reset Calculator     */
/****************************/
void Calc_Engine::reset() {
    m_state.expression.clear();
    m_state.display_value = "0";
    m_state.error         = "";
    m_result_shown        = false;
}

/****************************/
/*     Get Calculator State */
/****************************/
const Calc_State& Calc_Engine::state() const {
    return m_state;
}

/*****************************************/
/*     Get Calculator State (Mutable)    */
/*****************************************/
Calc_State& Calc_Engine::state() {
    return m_state;
}

/****************************/
/*       Key Handling       */
/****************************/
void Calc_Engine::handle_key( core::Action_Code code ) {
    m_state.error = "";

    if (m_result_shown && code != core::Action_Code::EQUALS) {
        switch (code) {
            case core::Action_Code::ADD:
            case core::Action_Code::SUBTRACT:
            case core::Action_Code::MULTIPLY:
            case core::Action_Code::DIVIDE:
            case core::Action_Code::POWER_N:
            case core::Action_Code::ANS:
                break;
            default:
                m_state.expression.clear();
                break;
        }
        m_result_shown = false;
    }

    switch (code) {
        // Digits and decimal
        case core::Action_Code::DIGIT_0:    case core::Action_Code::DIGIT_1:    case core::Action_Code::DIGIT_2:
        case core::Action_Code::DIGIT_3:    case core::Action_Code::DIGIT_4:    case core::Action_Code::DIGIT_5:
        case core::Action_Code::DIGIT_6:    case core::Action_Code::DIGIT_7:    case core::Action_Code::DIGIT_8:
        case core::Action_Code::DIGIT_9:    case core::Action_Code::DECIMAL:
        // Hex digits
        case core::Action_Code::HEX_A:      case core::Action_Code::HEX_B:      case core::Action_Code::HEX_C:
        case core::Action_Code::HEX_D:      case core::Action_Code::HEX_E:      case core::Action_Code::HEX_F:
        // Operators
        case core::Action_Code::ADD:        case core::Action_Code::SUBTRACT:   case core::Action_Code::MULTIPLY:
        case core::Action_Code::DIVIDE:     case core::Action_Code::POWER_N:    case core::Action_Code::POWER_2:
        case core::Action_Code::PERCENT:    case core::Action_Code::FACTORIAL:  case core::Action_Code::RECIPROCAL:
        case core::Action_Code::PAREN_OPEN: case core::Action_Code::PAREN_CLOSE:
        // Functions
        case core::Action_Code::SIN:        case core::Action_Code::COS:        case core::Action_Code::TAN:
        case core::Action_Code::ASIN:       case core::Action_Code::ACOS:       case core::Action_Code::ATAN:
        case core::Action_Code::LOG:        case core::Action_Code::LN:         case core::Action_Code::EXP:
        case core::Action_Code::SQRT:
        // Constants
        case core::Action_Code::PI:         case core::Action_Code::EULER:      case core::Action_Code::PHI: case core::Action_Code::TAU:
        // Bitwise
        case core::Action_Code::BIT_AND:    case core::Action_Code::BIT_OR:     case core::Action_Code::BIT_XOR:
        case core::Action_Code::BITSHIFT_LEFT: case core::Action_Code::BITSHIFT_RIGHT:
        case core::Action_Code::APPROX:
        case core::Action_Code::MAT_TRANSPOSE: case core::Action_Code::MAT_DET: case core::Action_Code::MAT_INV:
            try_insert(code);
            break;

        case core::Action_Code::NEGATE:
            if (!m_state.expression.empty()) {
                m_state.expression.insert(core::Action_Code::SUBTRACT);
                m_state.display_value = m_state.expression.render_string();
            }
            break;

        case core::Action_Code::CURSOR_LEFT:
            m_state.expression.cursor_left();
            break;

        case core::Action_Code::CURSOR_RIGHT:
            m_state.expression.cursor_right();
            break;

        case core::Action_Code::CURSOR_UP:
            m_state.expression.cursor_up();
            break;

        case core::Action_Code::CURSOR_DOWN:
            m_state.expression.cursor_down();
            break;

        case core::Action_Code::BACKSPACE:
            m_state.expression.backspace();
            m_state.display_value = m_state.expression.empty() ? "0" : m_state.expression.render_string();
            break;

        case core::Action_Code::CLEAR:
            m_state.expression.clear();
            m_state.display_value = "0";
            break;

        case core::Action_Code::ALL_CLEAR:
            reset();
            break;

        case core::Action_Code::MEM_STORE:
            {
                auto tree = Parser(m_state.expression.eval_string()).parse();
                if (tree) m_state.memory = tree->eval();
            }
            break;
        case core::Action_Code::MEM_RECALL:
            try_insert(core::Action_Code::EULER); // placeholder — MEM_RECALL inserts numeric string
            break;
        case core::Action_Code::MEM_ADD:
            {
                auto tree = Parser(m_state.expression.eval_string()).parse();
                if (tree) m_state.memory += tree->eval();
            }
            break;
        case core::Action_Code::MEM_CLEAR:
            m_state.memory = 0.0;
            break;

        case core::Action_Code::ANS:
            if (m_state.last_ast) {
                m_state.expression.insert_node(*m_state.last_ast);
                m_state.display_value = m_state.expression.render_string();
            }
            break;

        case core::Action_Code::EQUALS:
        case core::Action_Code::EVAL:
            evaluate();
            break;

        default:
            break;
    }
}

/****************************/
/*    Expression Insert     */
/****************************/
void Calc_Engine::try_insert( core::Action_Code code ) {
    LOG_DEBUG("try_insert: action_code=" + std::to_string(static_cast<int>(code)));
    m_state.expression.insert(code);
    m_state.display_value = m_state.expression.render_string();
    LOG_DEBUG("Expression after insert: " + m_state.display_value);
}

/****************************/
/*       Evaluation         */
/****************************/
void Calc_Engine::evaluate() {
    LOG_DEBUG("evaluate() called, expression empty: " + std::string(m_state.expression.empty() ? "yes" : "no"));
    if (m_state.expression.empty()) return;

    std::string expr_str = m_state.expression.eval_string();
    LOG_DEBUG("Evaluating expression: " + expr_str);
    Parser           p(expr_str);
    ast::Node::ptr_t tree = p.parse();

    if (!tree) {
        LOG_ERROR("Parse error | Expression: " + expr_str);
        m_state.error         = "Parse error";
        m_state.display_value = "Error";
        return;
    }

#ifdef __cpp_exceptions
    try {
#endif
        ast::Node::ptr_t result = tree->simplify();

        std::string result_str = result->to_string();
        LOG_DEBUG("Evaluation result: " + result_str);

        // Push to history before setting result
        m_state.history.push_front({expr_str, result_str});
        if (m_state.history.size() > Calc_State::MAX_HISTORY)
            m_state.history.pop_back();

        // Set expression to the result value so user can continue calculating
        if (auto* num_node = dynamic_cast<ast::Number_Node*>(result.get())) {
            m_state.expression.set_number(num_node->value());
        } else {
            // For non-numeric results, clear the expression
            m_state.expression.clear();
        }
        m_state.display_value = result_str;
        m_state.last_ast      = std::move(result);
        m_result_shown        = true;
#ifdef __cpp_exceptions
    } catch (const std::exception& ex) {
        LOG_ERROR("Evaluation error: " + std::string(ex.what()) + " | Expression: " + expr_str);
        m_state.error         = ex.what();
        m_state.display_value = "Error";
    }
#endif
}

/****************************/
/*     Math Layout Mode     */
/****************************/
void Calc_Engine::toggle_math_layout() {
    m_state.use_math_layout = !m_state.use_math_layout;
}

/****************************/
/*      Insert Matrix       */
/****************************/
void Calc_Engine::insert_matrix(int rows, int cols) {
    auto node = std::make_unique<ast::Matrix_Node>(rows, cols);
    m_state.expression.insert_node(*node);
    m_state.display_value = m_state.expression.render_string();
}

/****************************/
/*      Insert Vector       */
/****************************/
void Calc_Engine::insert_vector(int n) {
    auto node = std::make_unique<ast::Matrix_Node>(1, n);
    m_state.expression.insert_node(*node);
    m_state.display_value = m_state.expression.render_string();
}

/******************************/
/*      Result Shown Check    */
/******************************/
bool Calc_Engine::result_shown() const {
    return m_result_shown;
}

} // namespace ovb::math

