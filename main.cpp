#include <climits>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
long long max_line_chars = LLONG_MAX ; bool no_chars = false ; bool is_id_char ( char c ) { return std :: isalnum ( static_cast 
< unsigned char > ( c ) ) || c == '_' ; } std :: string generate_id ( int n , int L , const std :: vector < std :: string > & chars ) { int 
K = chars . size () ; std :: vector < std :: string > res_parts ( L , chars [ 0 ] ) ; for ( int i = L - 1 ; i >= 0 ; i -- ) { res_parts [ i ] = chars 
[ n % K ] ; n /= K ; } std :: string res ; for ( const auto & s : res_parts ) res += s ; return res ; } std :: map < std :: string , std :: string 
> create_mapping ( const std :: unordered_set < std :: string > & unique_items , const std :: vector < std :: string > & chars 
) { std :: map < std :: string , std :: string > mapping ; size_t K = chars . size () ; size_t N = unique_items . size () ; if ( N == 0 ) 
return mapping ; int L = 1 ; long long cap = K ; while ( cap < static_cast < long long > ( N ) ) { cap *= K ; L ++ ; } int idx = 0 ; for ( const 
auto & item : unique_items ) { mapping [ item ] = generate_id ( idx ++ , L , chars ) ; } return mapping ; } std :: vector < std :: string 
> tokenize_stream ( std :: ifstream & input , std :: vector < std :: string > & preprocessors ) { std :: vector < std :: string 
> tokens ; std :: string line ; const std :: vector < std :: string > operators = { ">>=" , "<<=" , "->*" , "->" , "::" , "++" , "--" 
, "==" , "!=" , "<=" , ">=" , "+=" , "-=" , "*=" , "/=" , "%=" , "|=" , "&=" , "^=" , "&&" , "||" , "<<" , ">>" , "+" , "-" , "*" , "/" , "%" 
, "=" , "<" , ">" , "!" , "&" , "|" , "^" , "~" , "?" , ":" , "," , "." , ".*" , "[]" , "()" } ; while ( std :: getline ( input , line ) ) { if ( 
line . empty () ) continue ; size_t first_char = line . find_first_not_of ( " \t" ) ; if ( first_char != std :: string :: npos 
&& line [ first_char ] == '#' ) { std :: string combined = line ; while ( ! combined . empty () && combined . back () == '\\') { combined 
. pop_back () ; std :: string next_line ; if ( ! std :: getline ( input , next_line ) ) break ; combined += " " + next_line ; } preprocessors 
. push_back ( combined ) ; continue ; } size_t i = 0 ; while ( i < line . size () ) { if ( std :: isspace ( static_cast < unsigned char 
> ( line [ i ] ) ) ) { i ++ ; continue ; } if ( line . compare ( i , 2 , "R\"" ) == 0 ) { size_t start = i ; size_t delim_pos = line . find ( '(' 
, i + 2 ) ; if ( delim_pos != std :: string :: npos ) { std :: string delim = line . substr ( i + 2 , delim_pos - ( i + 2 ) ) ; std :: string 
end_marker = ")" + delim + "\"" ; std :: string raw_content = line . substr ( i ) ; size_t end_pos = raw_content . find ( end_marker 
, delim_pos - i + 1 ) ; while ( end_pos == std :: string :: npos ) { std :: string next_line ; if ( ! std :: getline ( input , next_line 
) ) break ; raw_content += "\n" + next_line ; end_pos = raw_content . find ( end_marker ) ; } std :: string full_raw = raw_content 
. substr ( 0 , end_pos + end_marker . size () ) ; tokens . push_back ( full_raw ) ; i += full_raw . size () ; if ( full_raw . find ( 
'\n' ) != std :: string :: npos ) break ; continue ; } } if ( line [ i ] == '"' || line [ i ] == '\'' ) { char quote = line [ i ] ; std :: string 
t ; t += line [ i ++ ] ; while ( i < line . size () ) { t += line [ i ] ; if ( line [ i ] == quote && line [ i - 1 ] != '\\') { i ++ ; break ; } i ++ ; } 
tokens . push_back ( t ) ; continue ; } if ( line . compare ( i , 2 , "//" ) == 0 ) break ; bool matched = false ; for ( const auto & op : 
operators ) { if ( line . compare ( i , op . size () , op ) == 0 ) { tokens . push_back ( op ) ; i += op . size () ; matched = true ; break ; 
} } if ( matched ) continue ; if ( is_id_char ( line [ i ] ) ) { std :: string t ; while ( i < line . size () && is_id_char ( line [ i ] ) ) 
t += line [ i ++ ] ; tokens . push_back ( t ) ; continue ; } tokens . push_back ( std :: string ( 1 , line [ i ++ ] ) ) ; } } return tokens 
; } void obfuscate ( std :: ifstream & input , std :: ofstream & output , const std :: vector < std :: string > & chars , std :: string 
head = "" , int line_count = 5 ) { std :: vector < std :: string > preprocessors ; std :: vector < std :: string > tokens = tokenize_stream 
( input , preprocessors ) ; if ( ! head . empty () ) output << head << "\n" ; for ( const auto & p : preprocessors ) output << p << "\n" 
; int cnt = 0 ; long long line_chars = 0 ; if ( no_chars ) { for ( const auto & t : tokens ) { output << t << " " ; line_chars += t . size 
() ; if ( line_chars >= max_line_chars || ++ cnt % line_count == 0 ) { line_chars = cnt = 0 ; output << "\n" ; } } return ; } std :: 
unordered_set < std :: string > unique_tokens ( tokens . begin () , tokens . end () ) ; auto mapping = create_mapping ( unique_tokens 
, chars ) ; for ( const auto & m : mapping ) output << "#define " << m . second << " " << m . first << "\n" ; for ( const auto & t : tokens 
) { output << mapping [ t ] << " " ; line_chars += mapping [ t ] . size () ; if ( line_chars >= max_line_chars || ++ cnt % line_count 
== 0 ) { line_chars = cnt = 0 ; output << "\n" ; } } } bool is_raw_str_start ( const std :: string & s , size_t pos , std :: string & 
delim ) { if ( s . compare ( pos , 2 , "R\"" ) == 0 ) { size_t paren = s . find ( '(' , pos + 2 ) ; if ( paren != std :: string :: npos ) { delim 
= s . substr ( pos + 2 , paren - ( pos + 2 ) ) ; return true ; } } return false ; } std :: string normalize_line ( const std :: string & 
s ) { if ( s . empty () ) return "" ; std :: string result ; bool in_string = false ; char quote_char = 0 ; bool last_was_space = false 
; size_t start = s . find_first_not_of ( " \t\r\n" ) ; if ( start == std :: string :: npos ) return "" ; size_t end = s . find_last_not_of 
( " \t\r\n" ) ; for ( size_t i = start ; i <= end ; ++ i ) { char c = s [ i ] ; if ( ( c == '"' || c == '\'' ) && ( i == 0 || s [ i - 1 ] != '\\')) { if 
( ! in_string ) { in_string = true ; quote_char = c ; } else if ( c == quote_char ) { in_string = false ; } } if ( in_string ) { result 
+= c ; last_was_space = false ; } else { if ( std :: isspace ( static_cast < unsigned char > ( c ) ) ) { if ( ! last_was_space ) { result 
+= ' ' ; last_was_space = true ; } } else { result += c ; last_was_space = false ; } } } return result ; } void obfuscate_line ( 
std :: ifstream & input , std :: ofstream & output , const std :: vector < std :: string > & chars , std :: string head = "" , int line_count 
= 5 ) { std :: vector < std :: string > preprocessors ; std :: vector < std :: string > processed_parts ; std :: string line ; std 
:: string current_raw_block ; bool in_raw_string = false ; std :: string raw_delim ; while ( std :: getline ( input , line 
) ) { if ( ! in_raw_string ) { size_t first = line . find_first_not_of ( " \t" ) ; if ( first != std :: string :: npos && line [ first 
] == '#' ) { preprocessors . push_back ( line ) ; continue ; } size_t i = 0 ; bool found_raw = false ; while ( i < line . size () ) { if 
( line [ i ] == '"' || line [ i ] == '\'' ) { char q = line [ i ++ ] ; while ( i < line . size () && ( line [ i ] != q || line [ i - 1 ] == '\\')) i ++ 
; if ( i < line . size () ) i ++ ; continue ; } if ( line . compare ( i , 2 , "//" ) == 0 ) { line = line . substr ( 0 , i ) ; break ; } if ( is_raw_str_start 
( line , i , raw_delim ) ) { in_raw_string = true ; current_raw_block = line ; found_raw = true ; break ; } i ++ ; } if ( ! found_raw 
) { std :: string clean_line = normalize_line ( line ) ; if ( ! clean_line . empty () ) { processed_parts . push_back ( clean_line 
) ; } } } else { current_raw_block += "\n" + line ; std :: string end_marker = ")" + raw_delim + "\"" ; if ( line . find ( end_marker 
) != std :: string :: npos ) { in_raw_string = false ; processed_parts . push_back ( current_raw_block ) ; current_raw_block 
. clear () ; } } } if ( ! head . empty () ) output << head << "\n" ; for ( const auto & p : preprocessors ) output << p << "\n" ; int cnt 
= 0 ; long long line_chars = 0 ; if ( no_chars ) { for ( const auto & part : processed_parts ) { output << part << " " ; line_chars 
+= part . size () ; if ( line_chars >= max_line_chars || ++ cnt % line_count == 0 ) { line_chars = cnt = 0 ; output << "\n" ; } } return 
; } std :: unordered_set < std :: string > unique_items ( processed_parts . begin () , processed_parts . end () ) ; auto mapping 
= create_mapping ( unique_items , chars ) ; for ( const auto & m : mapping ) { std :: string val = m . first ; size_t pos = 0 ; while 
( ( pos = val . find ( '\n' , pos ) ) != std :: string :: npos ) { val . replace ( pos , 1 , " \\\n" ) ; pos += 3 ; } output << "#define " << 
m . second << " " << val << "\n" ; } for ( const auto & part : processed_parts ) { output << mapping [ part ] << " " ; line_chars += 
mapping [ part ] . size () ; if ( line_chars >= max_line_chars || ++ cnt % line_count == 0 ) { line_chars = cnt = 0 ; output << "\n" 
; } } } std :: ifstream expand_file ( std :: string file_path , std :: string out_path ) { std :: ifstream code_file ( file_path 
) ; std :: ofstream tmp_file ( out_path + ".tmp" ) ; std :: ofstream exp_file ( out_path + ".exp" ) ; if ( ! code_file . is_open 
() ) { std :: cout << "Code file open failed!" << std :: endl ; std :: cout << file_path << std :: endl ; exit ( 21 ) ; } if ( ! tmp_file 
. is_open () ) { std :: cout << "Tmp file open failed!" << std :: endl ; std :: cout << out_path + ".tmp" << std :: endl ; exit 
( 22 ) ; } if ( ! exp_file . is_open () ) { std :: cout << "Exp file open failed!" << std :: endl ; std :: cout << out_path + ".exp" 
<< std :: endl ; exit ( 23 ) ; } std :: string code_line ; while ( std :: getline ( code_file , code_line ) ) if ( code_line . substr 
( 0 , 8 ) == "#include" ) exp_file << code_line << std :: endl ; else tmp_file << code_line << std :: endl ; tmp_file . close () 
; exp_file . close () ; int res = 0 ; res |= system ( std :: string ( "g++ -E -P -x c++ \"" + out_path + ".tmp\" >>\"" + out_path 
+ ".exp\"" ) . data () ) ; res |= system ( std :: string ( "rm \"" + out_path + ".tmp\"" ) . data () ) ; if ( res ) exit ( 24 ) ; return 
std :: ifstream ( out_path + ".exp" ) ; } long long to_ll ( const std :: string str ) { long long res = 0 ; for ( auto c : str ) res *= 
10 , res += c - '0' ; return res ; } void chars_push ( std :: vector < std :: string > & chars , std :: string using_chars ) { for ( int 
i = 0 ; i < using_chars . size () ; i ++ ) { while ( i < using_chars . size () && using_chars [ i ] == ' ' ) i ++ ; std :: string str ; while 
( i < using_chars . size () && using_chars [ i ] != ' ' ) str . push_back ( using_chars [ i ++ ] ) ; if ( str . size () ) chars . push_back 
( str ) ; } } int main ( int argc , char * * argv ) { std :: string file_path , out_path ; std :: string head , using_chars = "OOO OO1 O1O O11" 
, using_chars_x [ 2 ] ; std :: vector < std :: string > chars , chars_x [ 2 ] ; int mode = 0 ; int line_count = 7 ; for ( int i = 1 ; i < argc 
; i ++ ) { std :: string arg = argv [ i ] ; if ( arg [ 0 ] == '-' ) { if ( arg . size () > 2 ) return 11 ; switch ( arg [ 1 ] ) { case 's' : if ( ++ i >= 
argc ) return 12 ; using_chars = argv [ i ] ; break ; case 'h' : if ( ++ i >= argc ) return 13 ; { std :: ifstream head_file ( argv [ i 
] ) ; std :: string line ; if ( ! head_file . is_open () ) { std :: cout << "head file open failed!" << std :: endl ; std :: cout 
<< argv [ i ] << std :: endl ; return 14 ; } while ( std :: getline ( head_file , line ) ) head += line + '\n' ; } break ; case 'l' : mode 
= 1 ; break ; case 'm' : if ( ++ i >= argc ) return 15 ; line_count = to_ll ( argv [ i ] ) ; break ; case 'x' : if ( ++ i - 1 >= argc ) return 
16 ; using_chars_x [ 0 ] = argv [ i ] ; using_chars_x [ 1 ] = argv [ ++ i ] ; mode = 2 ; break ; case 't' : mode = 3 ; break ; case 'n' : no_chars 
= true ; break ; case 'M' : if ( ++ i >= argc ) return 17 ; max_line_chars = to_ll ( argv [ i ] ) ; break ; default : std :: cout << "Unknown command!" 
<< std :: endl ; return 19 ; } } else { if ( file_path . size () == 0 ) file_path = arg ; else if ( out_path . size () == 0 ) out_path = 
arg ; else { std :: cout << "Unknown command!" << std :: endl ; return 18 ; } } } std :: ifstream exp_file = expand_file ( file_path 
, out_path ) ; std :: ofstream out_file ( out_path ) ; if ( mode < 2 ) { chars_push ( chars , using_chars ) ; if ( mode <= 0 ) obfuscate 
( exp_file , out_file , chars , head , line_count ) ; else obfuscate_line ( exp_file , out_file , chars , head , line_count 
) ; } else if ( mode == 2 ) { chars_push ( chars_x [ 0 ] , using_chars_x [ 0 ] ) ; chars_push ( chars_x [ 1 ] , using_chars_x [ 1 ] ) ; std 
:: ofstream tmp_file ( out_path + ".tmp" ) ; if ( ! tmp_file . is_open () ) { tmp_end : std :: cout << "tmp file open failed!" 
<< std :: endl ; return 31 ; } obfuscate ( exp_file , tmp_file , chars_x [ 0 ] , head , line_count ) ; tmp_file . close () ; std :: 
ifstream in_tmp_file ( out_path + ".tmp" ) ; if ( ! in_tmp_file . is_open () ) goto tmp_end ; obfuscate_line ( in_tmp_file 
, out_file , chars_x [ 1 ] , head , line_count ) ; in_tmp_file . close () ; if ( system ( std :: string ( "rm \"" + out_path + ".tmp\"" 
) . data () ) ) return 32 ; } else if ( mode == 3 ) { std :: string line ; while ( getline ( exp_file , line ) ) out_file << line << '\n' 
; } exp_file . close () ; out_file . close () ; if ( system ( std :: string ( "rm \"" + out_path + ".exp\"" ) . data () ) ) return 33 
; } 