/*ckwg +29
 * Copyright 2013-2015 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config_parser.h"
#include "token_expander.h"
#include "token_type_symtab.h"
#include "token_type_sysenv.h"
#include "token_type_env.h"
#include "token_type_config.h"

#include <vital/logger/logger.h>

#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
#else
#if BOOST_FILESYSTEM_VERSION == 2
#error "Only boost::filesystem version 3 is supported."
#endif
#endif

#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <cstring>
#include <cerrno>
#include <vector>
#include <fstream>
#include <cctype>

namespace kwiver {
namespace vital {

// trim from start
static inline std::string&
ltrim( std::string& s )
{
  s.erase( s.begin(), std::find_if( s.begin(), s.end(),
                                    std::not1( std::ptr_fun< int, int > ( std::isspace ) ) ) );
  return s;
}


// trim from end
static inline std::string&
rtrim( std::string& s )
{
  s.erase( std::find_if( s.rbegin(), s.rend(),
                         std::not1( std::ptr_fun< int, int > ( std::isspace ) ) ).base(), s.end() );
  return s;
}


// trim from both ends
static inline std::string&
trim( std::string& s )
{
  return ltrim( rtrim( s ) );
}

// ------------------------------------------------------------------

struct token_t
{
  enum type {
    TK_LHS = 1,
    TK_RHS,
    TK_ASSIGN,
    TK_EOL,
    TK_EOF
  };

  type type;
  std::string value;
};


// ------------------------------------------------------------------
/**
 * \brief Block context structure.
 *
 * This structure holds information about the block being
 * processed. An object of this type is allocated when a "block"
 * directive is encountered. Nested blocks are managed on a stack.
 */
struct block_context_t
{
  std::string m_block_name;     // block name taken from 'block' keyword
  std::string m_file_name;      // file where block started
  int m_start_line;              // line number of block directive in file

  std::string m_previous_context;  // previous block context
};


// ------------------------------------------------------------------
class config_parser::priv
{
public:
  priv()
    : m_line_number( 0 ),
      m_file_count( 0 ),
      m_parse_error( false ),
      m_symtab( new kwiver::vital::token_type_symtab( "LOCAL" ) ),
      m_config_block( kwiver::vital::config_block::empty_config() ), //+ nay need to set a name
      m_logger( kwiver::vital::get_logger( "config_parser" ) ),
      m_token_state(0)
  {
    m_token_expander.add_token_type( new kwiver::vital::token_type_env() );
    m_token_expander.add_token_type( new kwiver::vital::token_type_sysenv() );
    m_token_expander.add_token_type( new kwiver::vital::token_type_config( m_config_block.get() ) );
    m_token_expander.add_token_type( m_symtab );
  }


  // ------------------------------------------------------------------
  /**
   * \brief Process a single input file.
   *
   * This method is called to start processing a new file.
   *
   * \param file_path Path to the file
   *
   * \throws config_file_not_found_exception if file could not be opened
   */
  void process_file( config_path_t const&  file_path)
  {
    // Reset token parser since we are starting a new file
    m_token_state = 0;
    m_token_line.clear();

    m_line_number = 0;

    // Try to open the file
    std::ifstream in_stream( file_path.c_str() );
    if ( ! in_stream )
    {
      throw config_file_not_found_exception( file_path, std::strerror( errno ) );
    }

    // update file count
    ++m_file_count;

    // Get directory part of the input file
    const config_path_t config_file_dir( file_path.parent_path() );

    while ( true )
    {
      // Get next non-blank line. We are done of EOF is found.
      token_t token;
      get_token( in_stream, token );

      if ( token.type == token_t::TK_EOF )
      { // EOF found
        --m_file_count;

        if ( 0 == m_file_count )
        {
          // check to see if the block stack is empty.
          if ( 0 != m_block_stack.size() )
          {
            std::stringstream msg;

            msg << "Unclosed blocks left at end of file:\n";
            while ( 0 != m_block_stack.size() )
            {
              msg << "Block " << m_block_stack.back().m_block_name
                  << " - Started at " << m_block_stack.back().m_file_name
                  << ":" << m_block_stack.back().m_start_line
                  << std::endl;

              m_block_stack.pop_back();
            }

            LOG_ERROR( m_logger, msg.str() );
            m_parse_error = true;
          }

          if ( m_parse_error )
          {
            throw config_file_not_parsed_exception( file_path, "Errors in config file" );
          }
        } // end of main file EOF handling
        return;
      }

      // ------------------------------------------------------------------
      if ( token.value == "include" )
      {
        /*
         * Handle "include" <file-path>
         */
        const int current_line( m_line_number ); // save current line number

        LOG_DEBUG( m_logger, "Including file \"" << m_token_line << "\" at "
                  << file_path.string() << ":" << m_line_number );

        config_path_t filename = m_token_line;
        flush_line(); // force read of new line

        // Prepend current directory if file specified is not absolute.
        if ( ! boost::filesystem::path( filename ).is_absolute() )
        {
          filename = config_file_dir /filename;
        }

        process_file( filename ); // process included file
        m_line_number = current_line; // restore line number
        continue;
      }

      // ------------------------------------------------------------------
      if ( token.value == "block" )
      {
        /*
         * Handle "block" <block-name>
         */
        get_token( in_stream, token ); // get block name
        if ( token.type != token_t::TK_LHS )
        {
          // Unexpected token - syntax error
          LOG_ERROR( m_logger, "Invalid syntax in line \"" << m_last_line <<
                     "\" at " << file_path.string() << ":" << m_line_number );
          m_parse_error = true;

          flush_line(); // force starting a new line
          continue;
        }

        // Save current block context and start another
        block_context_t* block_ctxt = new block_context_t();
        block_ctxt->m_block_name = token.value; // block name
        block_ctxt->m_file_name = file_path.string(); // current file name
        block_ctxt->m_start_line = m_line_number;
        block_ctxt->m_previous_context = m_current_context;

        m_current_context += token.value + kwiver::vital::config_block::block_sep;

        LOG_DEBUG( m_logger, "Starting new block \"" << m_current_context
                  << "\" at " << file_path.string() << ":" << m_line_number );

        m_block_stack.push_back( block_ctxt );

        flush_line(); // force starting a new line

        continue;
      }

      // ------------------------------------------------------------------
      if ( token.value == "endblock" )
      {
        /*
         * Handled "endblock" keyword
         */
        flush_line(); // force starting a new line

        if ( m_block_stack.empty() )
        {
          std::stringstream reason;
          reason << "\"endblock\" found without matching \"block\" at "
                 << file_path.string() << ":" << m_line_number;

          throw config_file_not_parsed_exception( file_path, reason.str() );
        }

        // Restore previous block context
        m_current_context = m_block_stack.back().m_previous_context;
        m_block_stack.pop_back( );
        continue;
      }

      bool rel_path(false);
      if ( token.value == "relativepath" )
      {
        /*
         * Handle "relatiepath" <key> = <filepath>
         * This is a modifier for a config entry
         */
        rel_path = true;
        get_token( in_stream, token ); // get next token
      }

      // This is supposed to be an LHS token
      if ( token.type != token_t::TK_LHS )
      {
        // Unexpected token - syntax error
        LOG_ERROR( m_logger, "Invalid syntax in line \"" << m_last_line <<
                   "\" at " << file_path.string() << ":" << m_line_number );
        m_parse_error = true;

        flush_line(); // force starting a new line
        continue;
      }

      const std::string lhs( token.value );
      get_token( in_stream, token ); // get next token

      // This is supposed to be an assignment operator
      if ( token.type != token_t::TK_ASSIGN )
      {
        // Unexpected token - syntax error
        LOG_ERROR( m_logger, "Invalid syntax in line \"" << m_last_line <<
                   "\" at " << file_path.string() << ":" << m_line_number );
        m_parse_error = true;

        flush_line(); // force starting a new line
        continue;
      }

      const std::string op( token.value ); // save operator string
      get_token( in_stream, token ); // get next token

      // This is supposed to be the RHS
      if ( token.type != token_t::TK_RHS )
      {
        // Unexpected token - syntax error
        LOG_ERROR( m_logger, "Invalid syntax in line \"" << m_last_line <<
                   "\" at " << file_path.string() << ":" << m_line_number );
        m_parse_error = true;

        flush_line(); // force starting a new line
        continue;
      }

      if ( op == ":=" )
      {
        /*
         * Handle local symbol definition
         * <lhs> := <rhs>
         */
        std::string val;
        val = m_token_expander.expand_token( token.value );
        m_symtab->add_entry( lhs, val );
      }
      else if ( op == "=" )
      {
        /*
         * Handle config entry definition
         * <key> = <value>
         */
        kwiver::vital::config_block_key_t key = m_current_context + lhs;
        std::string val;
        val = m_token_expander.expand_token( token.value );

        // prepend our current directory if this is a path
        if ( rel_path )
        {
          config_path_t file = val;
          config_path_t full = config_file_dir / file;
          val = full.string();
        }

        // Add key/value to config
        LOG_DEBUG( m_logger, "Adding entry to config: \"" << key << "\" = \"" << val << "\"" );
        m_config_block->set_value( key, val );
      }

    } // end while
  }


  // ----------------------------------------------------------------
  /**
   * @brief Read a line from the stream.
   *
   * This method reads a line from the stream, removes comments and
   * trailing spaces. It also suppresses blank lines.  The line count
   * for the current file is updated.
   *
   * @param str[in]    Stream to read from
   * @param line[out]  Next non-blank line in the file or an empty string for eof.
   *
   * @return \b true if line returned, \b false if end of file.
   */
  bool get_line( std::istream& str, std::string& line )
  {
    while ( true )
    {
      if ( ! getline( str, line ) )
      {
        // read failed.
        return false;
      }

      ++ m_line_number; // count line number
      m_last_line = line; // save for error reporting

      trim( line ); // trim off spaces

      if ( line.size() == 0 )
      {
        // skip blank line
        continue;
      }

      // remove # comments
      size_t idx = line.find_first_of( "#" );
      if ( idx != std::string::npos )
      {
        line.erase( line.find_first_of( "#" ) );
        trim( line );

        // We may have made a blank line
        if ( line.size() == 0 )
        {
          // skip blank line
          continue;
        }
      }

      // There appears to be something left after removing comments
      // and trimming spaces. Return that to the caller.
      break;
    } // end while

    return true;
  }



  // ------------------------------------------------------------------
  /**
   * @brief Get next token from the input stream.
   *
   * @param[in] str Stream to read from
   * @param token[out] next token from line
   */
  void get_token( std::istream& str, token_t & token )
  {
    // Test for end of line while processing
    if ( (m_token_line.size() == 0) && (m_token_state != 0) )
    {
      token.type = token_t::TK_EOL; // end of line
      token.value.clear();
      m_token_state = 0;
      return;
    }

    switch (m_token_state)
    {
    case 0: // initial m_token_state, need input
      if ( ! get_line( str, m_token_line ) )
      {
        token.type = token_t::TK_EOF;
        token.value.clear();
        m_token_state = 0;
        return;
      }
      // FALL THROUGH

    case 1: // get next token LHS
    {
      // Chunk off next item in line. There could be multiple words
      // before '=' or no '=' on this line
      //+ problem with := matches foo:bar if terminates on : not :=
      // extend token

      // find ":="
      std::string::size_type idx = m_token_line.find_first_of( " \t=" );
      if (idx == m_token_line.npos ) // did not find delimiter
      {
        idx = m_token_line.size(); // use the whole string
      }

      // Look for ':=' operator
      if ( (idx > 1) && (m_token_line[idx-1] == ':' ) && (m_token_line[idx] == '=' ) )
      {
        --idx;
      }

      token.value = m_token_line.substr( 0, idx ); // get LHS token
      token.type = token_t::TK_LHS;

      m_token_line = m_token_line.substr( idx ); // remove token from input
      ltrim( m_token_line ); // get rid of leading spaces

      if ( ! isalnum( m_token_line[0] ) )
      {
        // have found an operator.
        m_token_state = 2;
      }
      else
      {
        m_token_state = 1;
      }
    }
    break;

    case 2: // expecting operator first char is non alnum and not space
    {
      std::string::size_type idx(0);
      if ( m_token_line[0] == ':' )
      {
        token.type = token_t::TK_ASSIGN;
        token.value = ":=";
        idx = 2;
      }
      else if ( m_token_line[0] == '=' )
      {
        token.type = token_t::TK_ASSIGN;
        token.value = "=";
        idx = 1;
      }
      else
      { // this is unexpected
        std::string::size_type idx = m_token_line.find_first_of( " \t" );
        if (idx == m_token_line.npos ) // did not find delimiter
        {
          idx = m_token_line.size(); // use the whole string
        }

        token.value = m_token_line.substr( 0, idx );

        ltrim( m_token_line ); // get rid of leading spaces
        token.type = token_t::TK_LHS;
        m_token_line = m_token_line.substr( idx ); // remove token from input
      }

      m_token_line = m_token_line.substr( idx ); // remove token from input
      ltrim( m_token_line ); // get rid of leading spaces

      m_token_state = 3; // go to RHS state
    }
    break;

    case 3: // process rhs
    {
      // RHS comes after assignment op and takes all remaining characters
      token.value = m_token_line;
      token.type = token_t::TK_RHS;

      // Clear input line and go to init state.
      m_token_line.clear();
      m_token_state = 0;
    }
    break;

    } // end switch

    //+ std::cout << "--- state: " << m_token_state << "   returning token: \"" << token.value<<"\"\n";
  }


  /**
   * @brief Flush remaining line in parser.
   *
   * This method causes a new line to be read from the file.
   */
  void flush_line()
  {
    m_token_state = 0;
  }


  // ------------------------------------------------------------------
  // -- member data --


  // nested block stack
  boost::ptr_vector< block_context_t > m_block_stack;

  // current block context with trailing sep ':'
  std::string m_current_context;

  // Last line read  from file - used for error reporting
  std::string m_last_line;

  // current line number of input file
  int m_line_number;

  // Recursion level counter for included files
  int m_file_count;

  // Set if a parse error is encountered in the process. This latches
  // the error but allows the parser to continue to find more errors.
  bool m_parse_error;

  // macro provider
  token_expander m_token_expander;
  token_type_symtab* m_symtab;

  // config block being created
  kwiver::vital::config_block_sptr m_config_block;

  kwiver::vital::logger_handle_t m_logger;

  // -- token extractor data
  int m_token_state;
  std::string m_token_line;
};


// ==================================================================

config_parser
::config_parser( config_path_t const& file_path )
  : m_config_file( file_path ),
    m_priv( new config_parser::priv() )
{
}


config_parser
::~config_parser()
{
}


void
config_parser
::parse_config()
{
  m_priv->process_file( m_config_file );
}


kwiver::vital::config_block_sptr
config_parser
::get_config() const
{
  return m_priv->m_config_block;
}


} } // end namespace
