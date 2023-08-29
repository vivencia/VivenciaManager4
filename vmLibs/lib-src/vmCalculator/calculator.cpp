#include "calculator.h"
#include "vmlibs.h"
#include "heapmanager.h"

#include <QtCore/QStack>
#include <QtCore/QVector>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>

//------------------------------------TOKEN-STACK-CLASS------------------------------------------
class Tokens : public QVector<Token>
{
public:
	Tokens () = default;
	inline Tokens ( const Tokens& t ) : QVector<Token> ( t ), m_valid ( t.valid () ) {}
	inline ~Tokens () {	this->clear (); }

	inline bool valid () const { return m_valid; }
	inline void setValid ( const bool v ) {	m_valid = v; }

protected:
	bool m_valid {true};
};

class TokenStack: public QVector<Token>
{
public:

	TokenStack () = default;

	inline bool isEmpty () const { return topIndex == 0; }
	inline uint itemCount () const { return topIndex; }
	inline Token pop () { return topIndex > 0 ? Token ( at ( static_cast<int>(--topIndex) ) ) : Token (); }
	inline void push ( const Token &token )
	{
		ensureSpace ();
		( *this ) [static_cast<int>(topIndex++)] = token;
	}
	
	inline const Token& top () const { return top ( 0 ); }
	inline const Token& top ( uint index ) const
	{
		return ( topIndex > index ) ? at ( static_cast<int>(topIndex - index - 1) ) : Token::null;
	}

private:
	inline void ensureSpace ()
	{
		//while ( topIndex >= unsigned ( size () ) ) resize ( size () + 10 );
		if ( static_cast<int>(topIndex) >= size () )
			resize ( static_cast<int>(topIndex) + 10 );
	}
	uint topIndex { 0 };
};

const Token Token::null;

// helper function: give operator precedence
// e.g. "+" is 1 while "*" is 3
static int opPrecedence ( Token::Op op )
{
	int prec = -1;
	switch ( op ) {
		case Token::Exclamation:
			prec = 8;
		break;
		case Token::Percent:
			prec = 8;
		break;
		case Token::Caret:
			prec = 7;
		break;
		case Token::Asterisk:
			prec = 5;
		break;
		case Token::Slash:
			prec = 6;
		break;
		case Token::Modulo:
			prec = 6;
		break;
		case Token::Div:
			prec = 6;
		break;
		case Token::Plus:
			prec = 3;
		break;
		case Token::Minus:
			prec = 3;
		break;
		case Token::RightPar:
			prec = 0;
		break;
		case Token::LeftPar:
			prec = -1;
		break;
		case Token::Semicolon:
			prec = 0;
		break;
		default:
			prec = -1;
		break;
	}
	return prec;
}

struct Opcode
{
	enum Operation { Nop = 0, Load, Add, Sub, Mul, Div, Neg };

	unsigned int type { Operation::Nop };
	int index { 0 };

	Opcode () = default;
	explicit Opcode ( const Operation t ): type ( t ) {}
	Opcode ( const Operation t, int i ): type ( t ) , index ( i ) {}
};

Token::Token ( const TokenType type, const QString& text, const int pos )
{
	m_type = type;
	m_text = text;
	m_pos = pos;
}

Token::Token ( const Token& token )
{
	m_type = token.m_type;
	m_text = token.m_text;
	m_pos= token.m_pos;
}

Token& Token::operator= ( const Token& token )
{
	// OPT_CS #2
	if ( this != &token )
	{
		m_type = token.m_type;
		m_text = token.m_text;
		m_pos = token.m_pos;
	}
	return *this;
}

Token::Op Token::asOperator () const
{
	return m_text.isEmpty () ? (InvalidOp) : (isOperator () ? matchOperator ( m_text.at ( 0 ) ) : InvalidOp);
}

// helper function: return operator of given token text. e.g. "*" yields Operator::Asterisk, and so on
Token::Op Token::matchOperator ( const QChar& p )
{
	Token::Op result = Token::InvalidOp;
	switch ( p.unicode () ) {
	case '+':
		result = Token::Plus;
		break;
	case '-':
		result = Token::Minus;
		break;
	case '*':
		result = Token::Asterisk;
		break;
	case '/':
		result = Token::Slash;
		break;
	case '^':
		result = Token::Caret;
		break;
	case ';':
		result = Token::Semicolon;
		break;
	case '(':
		result = Token::LeftPar;
		break;
	case ')':
		result = Token::RightPar;
		break;
	case '%':
		result = Token::Percent;
		break;
	case '!':
		result = Token::Exclamation;
		break;
	case '=':
		result = Token::Equal;
		break;
	default:
		result = Token::InvalidOp;
		break;
	}
	return result;
}
//------------------------------------TOKEN-STACK-CLASS------------------------------------------

//---------------------------------------vmCalculator---------------------------------------
// parsing state - Used in scan ()
static enum { Start = 0, Finish, Bad, InNumber, InDecimal } state;

struct vmCalculator::Private
{
	QStringList constants;
	QString error;
	QString expression;
	QString assignId;
	
	QVector<Opcode> codes;
		
	bool dirty { true };
	bool valid { false };
};

vmCalculator::vmCalculator ()
	: stc_private ( new Private )
{}

vmCalculator::~vmCalculator ()
{
	heap_del ( stc_private );
}

// Sets a new expression
// note that both the real lex and parse processes will happen later on
// when needed ( i.e. "lazy parse" ) , for example during evaluation.
void vmCalculator::setExpression ( const QString& expr )
{
	stc_private->expression = expr;
	stc_private->dirty = true;
	stc_private->valid = false;
	stc_private->error = QString ();
}

// Returns the validity. Note: empty expression is always invalid.
/*bool vmCalculator::isValid () const
{
	if ( stc_private->dirty ) {
	Tokens tokens = scan ( stc_private->expression );
	if ( !tokens.valid () )
		compile ( tokens );
		else
			stc_private->valid = false;
	}
	return stc_private->valid;
}*/

Tokens vmCalculator::scan ( const QString& expr )
{
	// result
	Tokens tokens;
	state = Start;

	int i ( 0 );
	QString ex = expr;
	QString tokenText;
	int tokenStart ( 0 );
	int op ( Token::InvalidOp );
	Token::TokenType type;
	QChar ch;

	// force a terminator
	ex.append ( QChar () );

	// main loop
	while ( state != Bad && state != Finish && i < ex.length () ) {
		ch = ex.at ( i );
		switch ( state ) {

		case Start:
			tokenStart = i;
			// skip any whitespaces
			if ( ch.isSpace () )
				++i;
			else if ( ch.isDigit () ) // check for number
				state = InNumber;
			else if ( ch == '.' || ch == ',' ) { // radix char ?
				tokenText.append ( ex.at ( i++ ) );
				state = InDecimal;
			}
			else if ( ch.isNull () ) // terminator character
				state = Finish;
			else { // look for operator match
				op = Token::matchOperator ( ch );
				if ( op != Token::InvalidOp ) {
					switch ( op ) { //refty
					case Token::LeftPar:
						type = Token::stxOpenPar;
						break;
					case Token::RightPar:
						type = Token::stxClosePar;
						break;
					case Token::Semicolon:
						type = Token::stxSep;
						break;
					default:
						type = Token::stxOperator;
					}
					++i;
					tokens.append ( Token ( type, QString ( ch ) ) );
				}
				else
					state = Bad;
			}
			break;

		case InNumber:
			if ( ch.isDigit () ) // consume as long as it's a digit
				tokenText.append ( ex.at ( i++ ) );
			else if ( ch == '.' || ch == ',' ) { // convert to '.'
				tokenText.append ( '.' );
				++i;
				state = InDecimal;
			}
			else { // we're done with integer number
				tokens.append ( Token ( Token::stxNumber, tokenText, tokenStart ) );
				tokenText = "";
				state = Start;
			}
			break;

			case InDecimal:
				if ( ch.isDigit () ) // consume as long as it's digit
					tokenText.append ( ex.at ( i++ ) );
				else
				{ // we're done with floating-point number
					tokens.append ( Token ( Token::stxNumber, tokenText, tokenStart ) );
					tokenText = "";
					state = Start;
				};
			break;

			case Bad: // bad bad bad
				tokens.setValid ( false );
			break;

			case Finish:
			break;
		};
	};
	return tokens;
}

void vmCalculator::compile ( const Tokens &tokens ) const
{
	// initialize variables
	stc_private->dirty = false;
	stc_private->valid = false;
	stc_private->codes.clear ();
	stc_private->constants.clear ();
	stc_private->error = QString ();

	// sanity check
	if ( tokens.count () == 0 )
		return;

	TokenStack syntaxStack;
	QStack<int> argStack;
	int argCount ( 1 );

	for ( int i = 0; i <= tokens.count (); ++i ) {
		// helper token: InvalidOp is end-of-expression
		const Token token = ( i < tokens.count () ) ? tokens.at ( i ): Token ( Token::stxOperator, "" );
		Token::TokenType tokenTokenType = token.type ();
		if ( tokenTokenType >= Token::stxOperator )
			tokenTokenType = Token::stxOperator;

		// unknown token is invalid
		if ( tokenTokenType == Token::stxUnknown )
			break;

		// for constants, push immediately to stack
		// generate code to load from a constant
		if ( tokenTokenType == Token::stxNumber ) {
			syntaxStack.push ( token );
			stc_private->constants.append ( token.asNumber () );
			stc_private->codes.append ( Opcode ( Opcode::Load, stc_private->constants.count () - 1 ) );
		}
		// special case for percentage
		else if ( tokenTokenType == Token::stxOperator &&
				  token.asOperator () == Token::Percent &&
				  syntaxStack.itemCount () >= 1 &&
				  !syntaxStack.top ().isOperator () ) {
			stc_private->constants.append ( QStringLiteral ( "0.01" ) );
			stc_private->codes.append ( Opcode ( Opcode::Load, stc_private->constants.count () - 1 ) );
			stc_private->codes.append ( Opcode ( Opcode::Mul ) );
		}
		else {
			// for any other operator, try to apply all parsing rules
			if ( tokenTokenType == Token::stxOperator && token.asOperator () != Token::Percent ) {
				// repeat until no more rule applies
				bool argHandled ( false );
				bool ruleFound ( false );
				while ( true )
				{
					ruleFound = false;
					// are we entering a function ? If token is operator, and stack already has: id ( arg
					//if ( !ruleFound && ! argHandled && tokenTokenType == Token::stxOperator && syntaxStack.itemCount () >= 3 ) {
					if ( !argHandled && tokenTokenType == Token::stxOperator && syntaxStack.itemCount () >= 3 ) {
						const Token& arg ( syntaxStack.top () );
						const Token& par ( syntaxStack.top ( 1 ) );
						//Token id = syntaxStack.top ( 2 );
						if ( !arg.isOperator () && par.asOperator () == Token::LeftPar ) {
							//ruleFound = true;
							argStack.push ( argCount );
							argCount = 1;
							break;
						}
					}
					// rule for parenthesis:( Y ) -> Y
					//if ( !ruleFound && syntaxStack.itemCount () >= 3 ) {
					if ( syntaxStack.itemCount () >= 3 )
					{
						const Token& right ( syntaxStack.top () );
						const Token& y ( syntaxStack.top ( 1 ) );
						const Token& left ( syntaxStack.top ( 2 ) );
						if ( right.isOperator () && ! y.isOperator () && left.isOperator () &&
								right.asOperator () == Token::RightPar && left.asOperator () == Token::LeftPar )
						{
							ruleFound = true;
							syntaxStack.pop ();
							syntaxStack.pop ();
							syntaxStack.pop ();
							syntaxStack.push ( y );
						}
					}
					// rule for function arguments, if token is , or ) id ( arg1 ; arg2 -> id ( arg
					if ( !ruleFound && syntaxStack.itemCount () >= 5 && ( token.asOperator () == Token::RightPar
							|| token.asOperator () == Token::Semicolon ) )
					{
						const Token& arg2 ( syntaxStack.top () );
						const Token& sep ( syntaxStack.top ( 1 ) );
						const Token& arg1 ( syntaxStack.top ( 2 ) );
						const Token& par ( syntaxStack.top ( 3 ) );
						if ( !arg2.isOperator () && sep.asOperator () == Token::Semicolon
								&& ! arg1.isOperator () && par.asOperator () == Token::LeftPar )
						{
							ruleFound = true;
							argHandled = true;
							syntaxStack.pop ();
							syntaxStack.pop ();
							++argCount;
						}
					}
					// rule for binary operator:A ( op ) B -> A
					// conditions: precedence of op >= precedence of token
					// action: push ( op ) to result
					// e.g. "A * B" becomes "A" if token is operator "+"
					// exception: for caret ( power operator ) , if op is another caret
					// then the rule doesn't apply, e.g. "2^3^2" is evaluated as "2^ ( 3^2 ) "
					if ( !ruleFound && syntaxStack.itemCount () >= 3 ) {
						const Token& b ( syntaxStack.top () );
						const Token& op ( syntaxStack.top ( 1 ) );
						const Token& a ( syntaxStack.top ( 2 ) );
						if ( ! a.isOperator () && ! b.isOperator () && op.isOperator () &&
								token.asOperator () != Token::LeftPar &&
								token.asOperator () != Token::Caret &&
								opPrecedence ( op.asOperator () ) >= opPrecedence ( token.asOperator () ) ) {
							ruleFound = true;
							syntaxStack.pop ();
							syntaxStack.pop ();
							syntaxStack.pop ();
							syntaxStack.push ( b );
							switch ( op.asOperator () )
							{
								// simple binary operations
								case Token::Plus:
									stc_private->codes.append ( Opcode ( Opcode::Add ) );
								break;
								case Token::Minus:
									stc_private->codes.append ( Opcode ( Opcode::Sub ) );
								break;
								case Token::Asterisk:
									stc_private->codes.append ( Opcode ( Opcode::Mul ) );
								break;
								case Token::Slash:
									stc_private->codes.append ( Opcode ( Opcode::Div ) );
								break;
								default:
								break;
							};
						}
					}
					// rule for unary operator:( op1 ) ( op2 ) X -> ( op1 ) X
					// conditions: op2 is unary, token is not ' ( '
					// action: push ( op2 ) to result
					// e.g."* - 2" becomes "*"
					if ( !ruleFound && token.asOperator () != Token::LeftPar && syntaxStack.itemCount () >= 3 ) {
						const Token& x ( syntaxStack.top () );
						const Token& op2 ( syntaxStack.top ( 1 ) );
						const Token& op1 ( syntaxStack.top ( 2 ) );
						if ( ! x.isOperator () && op1.isOperator () && op2.isOperator () &&
								( op2.asOperator () == Token::Plus || op2.asOperator () == Token::Minus ) ) {
							ruleFound = true;
							if ( op2.asOperator () == Token::Minus )
								stc_private->codes.append ( Opcode ( Opcode::Neg ) );
						}
						if ( ruleFound ) {
							syntaxStack.pop ();
							syntaxStack.pop ();
							syntaxStack.push ( x );
						}
					}
					// auxiliary rule for unary operator:( op ) X -> X
					// conditions: op is unary, op is first in syntax stack,
					// token is not ' ( ' or '^' or '!'
					// action: push ( op ) to result
					if ( !ruleFound && token.asOperator () != Token::LeftPar &&
							token.asOperator () != Token::Exclamation && syntaxStack.itemCount () == 2 ) {
						const Token& x ( syntaxStack.top () );
						const Token& op ( syntaxStack.top ( 1 ) );
						if ( ! x.isOperator () && op.isOperator () &&
								( op.asOperator () == Token::Plus || op.asOperator () == Token::Minus ) ) {
							ruleFound = true;
							if ( op.asOperator () == Token::Minus )
								stc_private->codes.append ( Opcode ( Opcode::Neg ) );
						}
						if ( ruleFound ) {
							syntaxStack.pop ();
							syntaxStack.pop ();
							syntaxStack.push ( x );
						}
					}
					if ( ! ruleFound )
						break;
				}

				// can't apply rules anymore, push the token
				if ( token.asOperator () != Token::Percent )
					syntaxStack.push ( token );
			}
		}
	}
	// syntaxStack must left only one operand and end-of-expression ( i.e. InvalidOp )
	stc_private->valid = false;
	if ( syntaxStack.itemCount () == 2 && syntaxStack.top ().isOperator () &&
			syntaxStack.top ().asOperator () == Token::InvalidOp &&
			!syntaxStack.top ( 1 ).isOperator () ) {
		stc_private->valid = true;
	}

	// bad parsing ? clean-up everything
	if ( !stc_private->valid ) {
		stc_private->constants.clear ();
		stc_private->codes.clear ();
	}
}

QString vmCalculator::autoFix ( const QString& expr ) const
{
	QString result;

	// strip off all funny characters
	for ( int c = 0; c < expr.length (); ++c )
	{
		if ( expr.at ( c ) >= QChar ( 32 ) )
		{
			result.append ( expr.at ( c ) );
		}
	}

	// no extra whitespaces at the beginning and at the end
	result = result.trimmed ();
	result.remove ( '=' );

	// automagically close all parenthesis
	const Tokens tokens = scan ( result );
	if ( tokens.count () ) {
		int par ( 0 );
		for ( int i = 0; i < tokens.count (); ++i ) {
			if ( tokens.at ( i ).asOperator () == Token::LeftPar )
				++par;
			else if ( tokens.at ( i ).asOperator () == Token::RightPar )
				--par;
		}

		if ( par < 0 )
			par = 0;

		// if the scanner stops in the middle, do not bother to apply fix
		const Token &lastToken = tokens.at ( tokens.count () - 1 );
		if ( lastToken.pos () + lastToken.text ().length () >= result.length () ) {
			while ( par-- )
				result.append ( ')' );
		}
	}
	return result;
}

void vmCalculator::eval ( QString& result )
{
	if ( stc_private->dirty ) {
		Tokens tokens = scan ( stc_private->expression );

		if ( !tokens.valid () ) { // invalid expression ?
			result = QCoreApplication::tr ( "Invalid Expression" );
			stc_private->error = result;
			return;
		}

		// variable assignment?
		stc_private->assignId = QString ();
		if ( tokens.count () > 2 && tokens.at ( 1 ).asOperator () == Token::Equal ) {
			stc_private->assignId = tokens.at ( 0 ).text ();
			tokens.erase ( tokens.begin () );
			tokens.erase ( tokens.begin () );
		}

		compile ( tokens );
	}

	QStack<double> stack;
	double val1 ( 0.0 ), val2 ( 0.0 );

	for ( int pc = 0; pc < stc_private->codes.count (); ++pc )
	{
		const Opcode &opcode (stc_private->codes.at ( pc ));
		const int index ( opcode.index );
		switch ( opcode.type )
		{
			// no operation
			case Opcode::Nop:
			break;

			// load a constant, push to stack
			case Opcode::Load:
				val1 = stc_private->constants.at ( index ).toDouble ();
				stack.push ( val1 );
			break;

			// unary operation
			case Opcode::Neg:
				if ( stack.count () < 1 )
				{
					result = QCoreApplication::tr ( "Invalid Expression" );
					stc_private->error = result;
					return;
				}
				val1 = stack.pop ();
				val1 = -val1;
				stack.push ( val1 );
			break;

			// binary operation: take two values from stack, do the operation,
			// push the result to stack
			case Opcode::Add:
				if ( stack.count () < 2 )
				{
					result = QCoreApplication::tr ( "Invalid Expression" );
					stc_private->error = result;
					return;
				}
				val1 = stack.pop ();
				val2 = stack.pop ();
				val2 += val1;
				stack.push ( val2 );
			break;

			case Opcode::Sub:
				if ( stack.count () < 2 )
				{
					result = QCoreApplication::tr ( "Invalid Expression" );
					stc_private->error = result;
					return;
				}
				val1 = stack.pop ();
				val2 = stack.pop ();
				val2 -= val1;
				stack.push ( val2 );
			break;

			case Opcode::Mul:
				if ( stack.count () < 2 )
				{
					result = QCoreApplication::tr ( "Invalid Expression" );
					stc_private->error = result;
					return;
				}
				val1 = stack.pop ();
				val2 = stack.pop ();
				val2 *= val1;
				stack.push ( val2 );
			break;

			case Opcode::Div:
				if ( stack.count () < 2 ) 
				{
					result = QCoreApplication::tr ( "Invalid Expression" );
					stc_private->error = result;
					return;
				}
				val1 = stack.pop ();
				val2 = stack.pop ();
				val2 /= val1;
				stack.push ( val2 );
			break;

			default:
			break;
		}
	}

	// more than one value in stack ? unsuccesfull execution...
	if ( stack.count () != 1 )
	{
		result = APP_TR_FUNC ( "Invalid Expression" );
		stc_private->error = result;
	}
	else
		//result = QString::number ( val2, 'f', 2 );
		result = QString::number ( stack.pop () , 'f', 2 );
}

//---------------------------------------vmCalculator---------------------------------------
