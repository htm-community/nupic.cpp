// ==============================================================
//
//  Copyright (C) 1995  William A. Rossi
//                      class RossiBigInt
// 
//  Copyright (C) 1999-2016  Alex Vinokur
//                           class BigInt 
//                           class BigInt::BaseBigInt 
//                           class BigInt::Vin
//                           upgrading class BigInt::Rossi
//                           class BigInt::Run
//                           class BigInt::Test
//                           class BigInt::TestVin
//                           class BigInt::TestRossi
//                           class BigInt::PerformanceTestVin
//                           class BigInt::PerformanceTestRossi 
//
//  ------------------------------------------------------------
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//  ------------------------------------------------------------
// 
//  mailto:alex DOT vinokur AT gmail DOT com
//  http://sourceforge.net/users/alexvn/
//
// ==============================================================


// ##############################################################
//
//  SOFTWARE : Class BigInt
//  FILE     : bigInt.h
//
//  DESCRIPTION : Declarations of the classes
//								* BigInt 
//								* BigInt::BaseBigInt 
//								* BigInt::Vin
//								* BigInt::Rossi
//								* BigInt::Run
//								* BigInt::Test
//								* BigInt::TestVin
//								* BigInt::TestRossi
//
// ##############################################################


#ifndef   __BIG_INT_H__
#define   __BIG_INT_H__

// include section
#include <cassert>
#include <climits>
#include <cmath>
#include <ctime>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <iomanip>
#include <utility>
#include <limits>
#include <algorithm>
#include <stdexcept>


// =================================================
static const std::size_t BIG_INT_MAJOR_VERSION = 10;
static const std::size_t BIG_INT_MINOR_VERSION =  2;
// =================================================



// ========================================
// BOOST
// --------------------------
// #define BIG_INT_USES_BOOST
// --------------------------
#ifdef BIG_INT_USES_BOOST
#include "boost/static_assert.hpp"
#endif
// ========================================





// =================================================
// ========== FUNCTION_NAME MACROS (BEGIN) =========


#ifdef FUNCTION_NAME
#undef FUNCTION_NAME
#endif

#ifdef UNDECORATED_FUNCTION_NAME
#undef UNDECORATED_FUNCTION_NAME
#endif

// --------------------------------------
// See
// http://predef.sourceforge.net/precomp.html
// --------------------------------------
#if defined _MSC_VER	// Windows
// #define FUNCTION_NAME __FUNCSIG__  // too long and not friendly representation
#define FUNCTION_NAME               __FUNCTION__
#define UNDECORATED_FUNCTION_NAME   __FUNCTION__
#elif defined __GNUC__	// g++ GNU C++
#define FUNCTION_NAME               __PRETTY_FUNCTION__
#define UNDECORATED_FUNCTION_NAME   __FUNCTION__
#elif defined __HP_aCC	// aCC on HP-UX
#define FUNCTION_NAME               __PRETTY_FUNCTION__
#define UNDECORATED_FUNCTION_NAME   __FUNCTION__
#elif defined __xlC__	// xlC on IBM AIX
#define FUNCTION_NAME               __FUNCTION__
#define UNDECORATED_FUNCTION_NAME   __func__
#elif defined __SUNPRO_CC	// SUN CC
#define FUNCTION_NAME               BOOST_CURRENT_FUNCTION  // Must be compiled with option  -features=extensions
#define UNDECORATED_FUNCTION_NAME   __func__                // Must be compiled with option  -features=extensions
#elif defined __INTEL_COMPILER	// Intel C/C++
#define FUNCTION_NAME               __PRETTY_FUNCTION__
#define UNDECORATED_FUNCTION_NAME   __FUNCTION__
#else
#define FUNCTION_NAME               "<Unable to detect FUNCTION_NAME>"
#define UNDECORATED_FUNCTION_NAME   "<Unable to detect UNDECORATED_FUNCTION_NAME>"
#endif


// -----------------------------------------------
#define SET_START_TEST_NAME(outstream) BigInt::Test::setTestName(outstream,  "START ", FUNCTION_NAME, __LINE__, std::numeric_limits<std::size_t>::max())
#define SET_FINISH_TEST_NAME(outstream) BigInt::Test::setTestName(outstream, "FINISH", FUNCTION_NAME, __LINE__, std::numeric_limits<std::size_t>::max())
#define SET_WORKING_TEST_NAME(outstream) BigInt::Test::setTestName(outstream, "Working", FUNCTION_NAME, __LINE__, std::numeric_limits<std::size_t>::max())
#define SET2_WORKING_TEST_NAME(outstream, t) BigInt::Test::setTestName(outstream, "Working", FUNCTION_NAME, __LINE__, t)
#define SET3_WORKING_TEST_NAME(outstream, t1, t2) BigInt::Test::setTestName(outstream, "Working", FUNCTION_NAME, __LINE__, t1, t2)
// #define SET_TBD(outstream) BigInt::Test::setTestName(outstream,  "TBD ", FUNCTION_NAME, __LINE__); return
#define SET_TBD(outstream) return
// #define SET_SKIP return

// #define WHERE_I_AM std::cerr << "[WHERE_I_AM] " << __FILE__ << ", Line-" << __LINE__ << std::endl 
// #define SHOW_IT(x) std::cerr << "[" << __FILE__ << ", Line-" << __LINE__ << "] " << #x << " = " << x << std::endl 


// ========== FUNCTION_NAME MACROS (END) ===========
// =================================================


// ===========================================
// ========== TIME MACROS (BEGIN) ============

#define START_TIME  s_startTime = std::clock(); assert(s_startTime != static_cast<std::clock_t>(-1))

#define END_TIME    s_endTime = std::clock(); assert(s_endTime != static_cast<std::clock_t>(-1)); assert (s_startTime <= s_endTime); s_elapsedTime = s_endTime - s_startTime
	                
#define DISPLAY_TIME(iii, out, bbb1, ppp2, nnn, op, ttt)  BigInt::Test::auxDisplayTime(iii, out, op, nnn, bbb1, ppp2, ttt)
						
// ========== TIME MACROS (END) ==============
// =================================================


// ===========================================
// ========== DISPLAY MACROS (BEGIN) =========

#define PRE_MSG(s, p)       s << p << "[" <<  __FILE__ << ", #" << std::dec << std::setw(3) << __LINE__ << "; " << UNDECORATED_FUNCTION_NAME << "] " << std::flush
#define SCREEN_MSG(s,p,x,t) { std::ostringstream oo_ss_ss; PRE_MSG(oo_ss_ss,p) << x << t << std::endl; s << std::flush << oo_ss_ss.str() << std::flush; if (&s != &std::cerr) { std::cerr << std::flush << oo_ss_ss.str() << std::flush; } }
#define FATAL_MSG(s, t)     SCREEN_MSG (s, "FATAL   ", "", t)
#define ERR_MSG(s, t)       SCREEN_MSG (s, "ERROR   ", "", t)
#define WARN_MSG(s, t)      SCREEN_MSG (s, "WARNING ", "", t)
#define INFO_MSG(s, t)      SCREEN_MSG (s, "INFO    ", "", t)
#define SUCCESS_MSG(s, t)   SCREEN_MSG (s, "SUCCESS ", "", t)

// ========== DISPLAY MACROS (END) ===========
// ===========================================



// ===========================================
// ========= ASSERTION MACROS (BEGIN) ========
#define ASSERTION(x) 	 assert(x); \
            if (!(x)) \
            { \
                std::cerr   << "[" \
			                << __FILE__ \
                            << ", " \
			                << __LINE__  \
			                << "] " \
                            << "statment " \
							<< "(" \
                            << #x \
							<< ") " \
							<< "is false; " \
                            << "assert() is off;" \
                            << " abort() will be called" \
                            << std::endl  \
                            << std::flush;  \
                abort(); \
			 }
// ========== ASSERTION MACROS (END) =========
// ===========================================


struct BigInt
{
    // -------------------
    // FORWARD DECLARATION
    // -------------------
    class Rossi;
 

    // --------
    // TYPEDEFS
    // --------

#ifndef UNIT32
#define UNIT64   // Default
#endif 

#if (defined UNIT32) && (defined UNIT64)
#error UNIT32 and UNIT64 may not be defined both
#endif

#if (!defined UNIT32) && (!defined UNIT64)
#error One of UNIT32 and UNIT64 must be defined
#endif

#ifdef UNIT64
#if (ULONG_MAX == 0xffffffffffffffff)   // (sizeof long == 8)
	typedef unsigned long Unit;	// Default
#else
	typedef unsigned long long Unit;	// Default
#endif // #if (sizeof(unsigned long) == 8)
#endif // #ifdef UNIT64

#ifdef UNIT32
#if (ULONG_MAX == 0xffffffffffffffff)   // (sizeof long == 8)
	typedef unsigned int Unit;
#else
	typedef unsigned long Unit;
#endif // #if (sizeof(unsigned long) == 8)
#endif // #ifdef UNIT32

    // ----------------
    // STATIC CONSTANTS
    // ----------------
    static const std::size_t DEC_DIGIT          = 10;
    static const std::size_t HEX_DIGIT          = 16;
    static const std::size_t NIBBLE_BIT         = 4;

	static const Unit UNIT_MAX = (((static_cast<Unit>(1) << (sizeof(Unit) * CHAR_BIT - 1)) - 1) << 1) + 1 ;
	static const Unit VIN_UNIT_MAX   = (UNIT_MAX >> 2);
    static const Unit UNIT_MSB        = static_cast<Unit>(1) << (sizeof(Unit) * CHAR_BIT - 1);

#ifdef UNIT64
	static const Unit VIN_BASE1  = 19;
	static const Unit VIN_BASE2  = 1000000000000000000;  // pow (10, (VIN_BASE1 - 1))  
#else
	static const Unit VIN_BASE1  = 10;
    static const Unit VIN_BASE2  = 1000000000;  // pow (10, (VIN_BASE1 - 1))                                 
#endif // ifdef UNIT64


	static const Unit VIN_SUB_BASE2    = VIN_BASE2 - 1; 
	static const Unit VIN_OVER_BASE2   = static_cast<Unit>(0x3) << (sizeof (Unit) * CHAR_BIT - 2);  // VIN_OVER_BASE2 > VIN_SUB_BASE2


	static const std::string s_strHelp;
	static const std::string s_strDemo;
	static const std::string s_strTest;
	static const std::string s_strTime;

    // -----------------------------------------
#ifdef BIG_INT_USES_BOOST
    BOOST_STATIC_ASSERT (!(VIN_BASE2 >= VIN_UNIT_MAX));
    BOOST_STATIC_ASSERT (VIN_BASE1 * (VIN_BASE2/VIN_BASE1 + 1) < VIN_UNIT_MAX);
    BOOST_STATIC_ASSERT (!(VIN_BASE2 != (VIN_SUB_BASE2 + 1)));
    BOOST_STATIC_ASSERT (VIN_OVER_BASE2 > VIN_SUB_BASE2);

    BOOST_STATIC_ASSERT(
                            (sizeof(Unit) == 4) && ((UNIT_MSB == static_cast<Unit>(0x80000000)))
                            ||
                            (sizeof(Unit) == 8) && ((UNIT_MSB == ((static_cast<Unit>(0x80000000) << 31) << 1)))
                       );


	// ---------------------------------------------
	// This assert is to avoid possible problematic situation for Windows 64-bits where
	//  sizeof (std::size_t) == 8, but sizeof (unsigned long) == 4
	//  If ASSERTION() failed then "typedef Unit" should be changed
	// BOOST_STATIC_ASSERT (sizeof(std::size_t) <= sizeof(Unit));
	// ---------------------------------------------

#endif // BIG_INT_USES_BOOST


#ifdef __cpp_static_assert
    static_assert (!(VIN_BASE2 >= VIN_UNIT_MAX), "(!(VIN_BASE2 >= VIN_UNIT_MAX)) failed");
    static_assert ((VIN_BASE1 * (VIN_BASE2/VIN_BASE1 + 1) < VIN_UNIT_MAX), "(VIN_BASE1 * (VIN_BASE2/VIN_BASE1 + 1) < VIN_UNIT_MAX) failed");
    static_assert (!(VIN_BASE2 != (VIN_SUB_BASE2 + 1)), "(!(VIN_BASE2 != (VIN_SUB_BASE2 + 1))) failed");
    static_assert (VIN_OVER_BASE2 > VIN_SUB_BASE2, "(VIN_OVER_BASE2 > VIN_SUB_BASE2) failed");

    static_assert(
                            (
								((sizeof(Unit) == 4) && ((UNIT_MSB == static_cast<Unit>(0x80000000))))
								||
								((sizeof(Unit) == 8) && ((UNIT_MSB == ((static_cast<Unit>(0x80000000) << 31) << 1))))
							),
							"((sizeof(Unit) == 4) && ((UNIT_MSB == static_cast<Unit>(0x80000000)))|| (sizeof(Unit) == 8) && ((UNIT_MSB == ((static_cast<Unit>(0x80000000) << 31) << 1)))) failed"
                       );

	// ---------------------------------------------
	// This assert is to avoid possible problematic situation for Windows 64-bits where
	//  sizeof (std::size_t) == 8, but sizeof (unsigned long) == 4
	//  If ASSERTION() failed then "typedef Unit" should be changed
	// static_assert (sizeof(std::size_t) <= sizeof(Unit), "(sizeof(std::size_t) <= sizeof(Unit)) failed");
	// ---------------------------------------------
#endif // __cpp_static_assert

    // -----------------------------------------
    // Public Methods
    static bool assertCheck(const bool i_preCheck = false);
    static void showVersion(std::ostream& o_stream);
	static void showSystem(std::ostream& o_stream);
	static void showCompiler(std::ostream& o_stream);
	static void showEnvironment(std::ostream& o_stream);
	static void showCompilationOptions(std::ostream& o_stream);

    // -----------------------------------
    template <typename T>
    static std::string toString(const T& t)
    {
        std::ostringstream oss;
        oss << t;
        return oss.str();
    }
    
     // -------------------------------
    template<typename T, std::size_t N> 
    static std::size_t getArrayElems(const T(&)[N]) 
    { 
        return N; 
    } 

    // -------------------------------
    template<typename T, std::size_t N> 
    static std::size_t getArrayElems(T(&)[N]) 
    { 
        return N; 
    } 

    // -------------------------------
    template<typename T, std::size_t N> 
    static std::size_t getArraySizeInBytes(const T(&a)[N]) 
    { 
        return getArrayElems(a) * sizeof (T); 
    }

    // -------------------------------
    template<typename T, std::size_t N> 
    static std::size_t getArraySizeInBytes(T(&a)[N]) 
    { 
        return getArrayElems(a) * sizeof (T); 
    } 


    // -------------------------------
    template<typename T, std::size_t N> 
    static std::vector<T> array2vector(const T(&a)[N]) 
    { 
        return std::vector<T> (a, a + getArrayElems (a));
    } 

    // --------------------------------
    template<typename T, std::size_t N> 
    static std::vector<T> array2vector(T(&a)[N]) 
    { 
        return std::vector<T> (a, a + getArrayElems (a));
    } 

    // ------------------------------------
    template<typename T> 
    static std::vector<T> elem2vector(const T& e) 
    { 
        return std::vector<T> (&e, &e + 1);
    } 

    // ------------------------------------
    template<typename T> 
    static std::vector<T> elem2vector(T& e) 
    { 
        return std::vector<T> (&e, &e + 1);
    } 

    // ---------------------------------------------------------
    template<typename K, typename T, std::size_t N> 
    static std::map<K, T> array2map(const std::pair<K,T>(&a)[N]) 
    { 
        return std::map<K, T> (a, a + getArrayElems (a));
    } 

    // ---------------------------------------------------------
    template<typename K, typename T, std::size_t N> 
    static std::map<K, T> array2map(std::pair<K,T>(&a)[N]) 
    { 
        return std::map<K, T> (a, a + getArrayElems (a));
    } 

    // ------------------------------------
    template<typename K, typename T>
    static std::map<K, T> pair2map(const std::pair<K,T>& e) 
    { 
        return std::map<K, T> (&e, &e + 1);
    } 

	// ------------------------------
	static Unit get_VIN_BASE1 (const Unit i_unitMax);
	static Unit get_VIN_BASE2 (const Unit i_base1);



// ==============
class BaseBigInt
// ==============
{

public:
    // --- Public Methods 
    std::size_t getUnitsSize () const;
	std::size_t getHexDigitsInUnits () const;
    void        maximize ();

protected :
    // --- Protected Data Members ---
    std::vector<Unit>                  m_units;      // BigHexDigit is Unit
    static std::map<char, std::size_t>  ms_hex2dec;

    // --- Protected Methods 
    void showUnits (std::ostream& o_stream) const;
    virtual ~BaseBigInt () = 0;




public :
    // --- Public Methods ---
	virtual std::size_t getActualHexDigits () const = 0;
	virtual std::size_t getActualDecDigits () const = 0;

    bool  isEmpty () const;

};



// ==============
class Vin : public BaseBigInt
// ==============
{
private :
    // -------
    // FRIENDS
    // -------
    friend std::ostream& operator<< (std::ostream& o_os, const Vin& i_ins);


private :
    // --- Private Data Members ---
    static Unit s_carry;

    // --- Private Methods ---
    static Unit    addUnits(Unit n1, Unit n2);
    bool           initViaString (const std::string& i_arg, std::size_t i_digitBase);

public :
    // ----------------------
    // --- Public Methods ---
    // ----------------------
    // --- Constructors ---
    explicit Vin ();
    explicit Vin (Unit i_unit);
    explicit Vin (const std::string& i_arg, std::size_t i_digitBase);
    explicit Vin (const Rossi& i_arg);

    // --- Aux methods  ---

	// -------------------------
	// toUnit () has sense only while m_units.size() == 1
    Unit toUnit () const;
    // operator Unit () const;
	// -------------------------

	// --------------------------
	// Precision of toDouble() and fromDouble() depends on std::numeric_limits<long double>::digits10 
	long double toDouble () const;
	static Vin fromDouble (const long double& i_double);
	// --------------------------

    // --- Show functions ---
    std::string toStrHex (const std::string& i_prefix = "") const;
    std::string toStr0xHex () const;
    std::string toStrDec (const std::string& i_prefix = "") const;

	std::size_t getActualHexDigits () const;
	std::size_t getActualDecDigits () const;


    // --- General purpose mathematical methods ---
    Vin operator+ (const Vin& i_arg) const;
    Vin operator* (Unit i_arg) const;


    // --- Comparison operators ---
    bool operator== (const Vin& i_arg) const;
    bool operator!= (const Vin& i_arg) const;
    bool operator<  (const Vin& i_arg) const;
    bool operator>  (const Vin& i_arg) const;
    bool operator<= (const Vin& i_arg) const;
    bool operator>= (const Vin& i_arg) const;


    // ---- Service methods ---
    void showUnits(std::ostream& o_stream) const;

};


// ============================
class Rossi : public BaseBigInt
// ============================
{
private :
    // -------
    // FRIENDS
    // -------
    friend std::ostream& operator<< (std::ostream& o_os, const Rossi& i_ins);

private :
    // --- Private Methods ---
    void  resizeUnits (std::size_t i_size);
    void  truncateUnits ();
    void  smartTruncateUnits ();
    bool  backUnitIsNull () const;
    bool  initViaString (const std::string& i_arg, std::size_t i_digitBase);


public :
    // ----------------------
    // --- Public Methods ---
    // ----------------------
    // --- Constructors ---
    explicit Rossi ();
    explicit Rossi (Unit i_unit);
    explicit Rossi (const std::string& i_arg, std::size_t i_digitBase);
    explicit Rossi (const Vin& i_arg);
	explicit Rossi (const std::size_t i_noOfUnits, Unit i_internalUnits, Unit i_backUnit, const std::string& i_msg = std::string());


    // --- Aux methods ---

	// -----------------------------------
	// toUnit () has sense only while m_units.size() == 1 
    Unit toUnit () const;
    // operator Unit () const;
	// -----------------------------------

	// ------------------------------------
	// Precision of toDouble() and fromDouble() depends on std::numeric_limits<long double>::digits10 
	long double toDouble () const;
	static Rossi fromDouble (const long double& i_double);



    // --- General purpose mathematical methods ---

    // Rossi& operator= (Unit i_arg);
    Rossi  operator+ (const Rossi& i_arg);
    Rossi  operator+ (Unit i_arg);
    Rossi  operator* (Rossi i_arg) const;
    Rossi  operator* (Unit i_arg) const;
    //  Rossi& Rossi::operator*= (Rossi i_arg);
    Rossi  operator/ (const Rossi& i_arg) const;
    Rossi  operator% (const Rossi& i_arg) const;
    Rossi  divide(const Rossi& i_dividend, const Rossi& i_divisor, Rossi* o_remainder) const;
    Rossi& operator+= (const Rossi& i_arg);
    Rossi  operator++ (int); // Post increment operator
    Rossi& operator++ ();    // Pre increment operator
    Rossi  operator-  (const Rossi& i_arg);
    Rossi  operator-  ();
    Rossi  operator-- (int); // Post decrement operator
    Rossi& operator-- ();    // Pre decrement operator
    Rossi& operator-= (const Rossi& i_arg);
    Rossi  sqrt();
	Rossi  naive_sqrt();
	static Rossi  pow(const Rossi& i_base, const Rossi& i_exp, const bool i_iterative = true);
	static Rossi  pow(const Rossi& i_base, const Unit i_exp, const bool i_iterative = true);
	static Rossi  naive_pow_iterative(const Rossi& i_base, const Rossi& i_exp);
	static Rossi  naive_pow_recursive(const Rossi& i_base, const Rossi& i_exp);


    // --- Bitwise boolean operators ---
    Rossi  operator&   (const Rossi& i_arg);
    Rossi  operator|   (const Rossi& i_arg);
    Rossi  operator^   (const Rossi& i_arg);
    Rossi& operator&=  (const Rossi& i_arg);
    Rossi& operator|=  (const Rossi& i_arg);
    Rossi& operator^=  (const Rossi& i_arg);
    Rossi  operator~   ();
    Rossi  operator>>  (std::size_t i_shift);
    Rossi  operator<<  (std::size_t  i_shift);
    Rossi& operator>>= (std::size_t  i_shift);
    Rossi& operator<<= (std::size_t  i_shift);



    // --- Comparison operators ---
    bool operator== (const Rossi& i_arg) const;
    bool operator!= (const Rossi& i_arg) const;
    bool operator<  (const Rossi& i_arg) const;
    bool operator>  (const Rossi& i_arg) const;
    bool operator<= (const Rossi& i_arg) const;
    bool operator>= (const Rossi& i_arg) const;

    // --- Show functions ---
    std::string toStrHex (const std::string& i_prefix = "") const;
    std::string toStr0xHex () const;
    std::string toStrDec (const std::string& i_prefix = "") const;

	std::size_t getActualHexDigits () const;
	std::size_t getActualDecDigits () const;


    // ---- Service methods ---
    void showUnits(std::ostream& o_stream) const;


};


// ============================
struct Run
{
    static int  mainBigInt(int argc, char** argv); 
    static int  mainBigInt(std::ostream& o_stream, const std::vector<std::string>& i_args); 
	static void showTime(std::ostream& o_stream, const std::string& i_msg);
	static void showCommandLine(std::ostream& o_stream, int argc, char** argvs);
	static void showHelp(std::ostream& o_stream, const std::vector<std::string>& i_args);
	static void showHelp(std::ostream& o_stream, const std::string& i_exeFileName = std::string()); 
    static bool checkCommandLine(std::ostream& o_stream, const std::vector<std::string>& i_args); 
    static std::map<std::string, std::string> getSampleAllowedOperation(); 
    static void runRossiInteractiveSample(std::ostream& o_stream, const std::vector<std::string>& i_args); 
    static void applicationSimpleSample(std::ostream& o_stream);  
};


// ============================
struct Test
{
	
	static std::size_t s_returnValue;

	static std::clock_t s_startTime;
	static std::clock_t s_endTime;
	static std::clock_t s_elapsedTime;
	static std::ostringstream s_oss;

	static const std::clock_t s_clockMinThreshold; 

	// ---------------------------
    static std::vector<std::string> fillTestInputHexStr();

	static void setTestName(std::ostream& o_stream, const std::string& i_txt, const std::string& i_funcName, const std::size_t i_lineNo, const std::size_t i_counter = std::numeric_limits<std::size_t>::max(), const std::size_t i_total = std::numeric_limits<std::size_t>::max());
	
    static void testDisplayBigInts (std::ostream& o_stream);
    static void testMain (std::ostream& o_stream, const std::vector<std::string>& i_args = std::vector<std::string>());

	static void auxDisplayTime(
						const std::size_t	i_serialNo, 
						std::ostream&		o_stream, 
						const std::string&	i_operationName,
						const std::size_t	i_totalOperations, 
						const BaseBigInt&	i_ins1, 
						const BaseBigInt*	i_ptr2,
						const clock_t		i_elapsedTime
						);
};

struct TestRossi : public Test
{
public:
    static std::vector<BigInt::Unit> fillTestInputUnit();

    static std::vector<std::pair<BigInt::Rossi, BigInt::Rossi> > fillTestInputPairsRossiRossi();
    static std::vector<std::pair<BigInt::Rossi, BigInt::Unit> > fillTestInputPairsRossiUnit();

    // ---------------------------------
    static void testAll (std::ostream& o_stream);
    static void testOperatorAdd (std::ostream& o_stream);
    static void testOperatorAddAssign (std::ostream& o_stream);
    static void testOperatorSubtraction (std::ostream& o_stream);
    static void testOperatorSubtractionAssign (std::ostream& o_stream);
    static void testOperatorMultiplication1 (std::ostream& o_stream);
    static void testOperatorMultiplication2 (std::ostream& o_stream);
    static void testOperatorDivision (std::ostream& o_stream);
    static void testOperatorReminder (std::ostream& o_stream);
    static void testOperatorLess (std::ostream& o_stream);
	static void testSqrt (std::ostream& o_stream);
	static void testDoubleAll (std::ostream& o_stream);
	static void testPowAll (std::ostream& o_stream);
	static void testPrintAll (std::ostream& o_stream);
    static void testMaxUnits (std::ostream& o_stream);
    static void testMaxMultiplication (std::ostream& o_stream);
	static void testTryCatch (std::ostream& o_stream);
	

	// ----------------------------------

	static void testAssertAll (std::ostream& o_stream);
    static void testAssertOperatorAdd (std::ostream& o_stream);
    static void testAssertOperatorAddAssign (std::ostream& o_stream);
    static void testAssertOperatorSubtraction (std::ostream& o_stream);
    static void testAssertOperatorSubtractionAssign (std::ostream& o_stream);
    static void testAssertOperatorMultiplication1 (std::ostream& o_stream);
    static void testAssertOperatorMultiplication2 (std::ostream& o_stream);
    static void testAssertOperatorDivision (std::ostream& o_stream);
    static void testAssertOperatorReminder (std::ostream& o_stream);
    static void testAssertOperatorLess (std::ostream& o_stream);
	static void testAssertSqrt (std::ostream& o_stream);
	static void testAssertDoubleAll (std::ostream& o_stream);
	static void testAssertPowAll (std::ostream& o_stream);
	static void testAssertPrintAll (std::ostream& o_stream);
    

private:
	static void testDouble(std::ostream& o_stream, const BigInt::Rossi& i_bigIntValue, const std::size_t i_serialNo = std::numeric_limits<std::size_t>::max());
	static void testPow(std::ostream& o_stream, const BigInt::Rossi& i_base, const BigInt::Rossi& i_exp);
	static void testPow(std::ostream& o_stream, const BigInt::Rossi& i_base, const BigInt::Unit i_exp);

	static void testAssertDouble(std::ostream& o_stream, const BigInt::Unit i_ulongValue, std::size_t& io_count);
	static void testAssertPow(std::ostream& o_stream, const BigInt::Unit i_base, const BigInt::Unit i_exp, std::size_t& io_count);
    
};


struct PerformanceTestRossi : public Test
{
public:
    static std::vector<BigInt::Unit> fillTestInputUnit();

    static std::vector<std::pair<BigInt::Rossi, BigInt::Rossi> > fillTestInputPairsRossiRossi();
    static std::vector<std::pair<BigInt::Rossi, BigInt::Unit> > fillTestInputPairsRossiUnit();

    // ---------------------------------
    static void performanceTestAll (std::ostream& o_stream);
    static void performanceTestOperatorAdd (std::ostream& o_stream);
    static void performanceTestOperatorAddAssign (std::ostream& o_stream);
    static void performanceTestOperatorSubtraction (std::ostream& o_stream);
    static void performanceTestOperatorSubtractionAssign (std::ostream& o_stream);
    static void performanceTestOperatorMultiplication1 (std::ostream& o_stream);
    static void performanceTestOperatorMultiplication2 (std::ostream& o_stream);
    static void performanceTestOperatorDivision (std::ostream& o_stream);
    static void performanceTestOperatorReminder (std::ostream& o_stream);
    static void performanceTestOperatorLess (std::ostream& o_stream);
	static void performanceTestSqrt (std::ostream& o_stream);
	static void performanceTestDouble (std::ostream& o_stream);
	static void performanceTestPow (std::ostream& o_stream);
    static void performanceTestMaxUnits (std::ostream& o_stream);
    static void performanceTestMaxMultiplication (std::ostream& o_stream);
    static void performanceTestTryCatch (std::ostream& o_stream);

};




struct TestVin : public Test
{
    static std::vector<BigInt::Unit> fillTestInputUnit();

    static std::vector<std::pair<BigInt::Vin, BigInt::Vin> > fillTestInputPairsVinVin();
    static std::vector<std::pair<BigInt::Vin, BigInt::Unit> > fillTestInputPairsVinUnit();


    // ---------------------------------
    static void testAll (std::ostream& o_stream);
    static void testOperatorAdd (std::ostream& o_stream);
    static void testOperatorMultiplication (std::ostream& o_stream);
    static void testOperatorLess (std::ostream& o_stream);
    static void testMaxUnits (std::ostream& o_stream);

	// ---------------------------------
    static void testAssertAll (std::ostream& o_stream);
    static void testAssertOperatorAdd (std::ostream& o_stream);
    static void testAssertOperatorLess (std::ostream& o_stream);
    
};



struct PerformanceTestVin : public Test
{
    static std::vector<BigInt::Unit> fillTestInputUnit();

    static std::vector<std::pair<BigInt::Vin, BigInt::Vin> > fillTestInputPairsVinVin();
    static std::vector<std::pair<BigInt::Vin, BigInt::Unit> > fillTestInputPairsVinUnit();


    // ---------------------------------
    static void performanceTestAll (std::ostream& o_stream);
    static void performanceTestOperatorAdd (std::ostream& o_stream);
    static void performanceTestOperatorMultiplication (std::ostream& o_stream);
    static void performanceTestOperatorLess (std::ostream& o_stream);
    static void performanceTestMaxUnits (std::ostream& o_stream);


};




}; // class BigInt



#endif    // __BIG_INT_H__
