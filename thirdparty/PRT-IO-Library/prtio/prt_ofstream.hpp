/**
 * Copyright 2012 Thinkbox Software Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file contains the definition of a stream for writing prt files. For more information on the
 * PRT file specification, refer to http://www.thinkboxsoftware.com/krak-prt-file-format/
 */

#pragma once

#include <prtio/prt_ostream.hpp>
#include <prtio/detail/prt_header.hpp>
#include <fstream>
#include <zlib.h>

namespace prtio{

/**
 * This class implements the prt_istream interface, for reading particles from a file.
 */
class prt_ofstream : public prt_ostream{
	std::string m_filePath; //The path to the PRT file.
	std::ofstream m_fout;   //The stream that is writing bytes to the file.
	z_stream m_zstream;     //The zlib stream that is compressing particles for writing to the file.

	char* m_buffer;           //A temporary buffer for storing the compressed file data before being flushed to disk.
	std::size_t m_bufferSize; //The size of 'm_buffer' in bytes.

	detail::prt_int64 m_particleCount; //The number of particles written so far.

	std::ostream::streampos m_countLocation; //The location that we need to write the final particle count to.

private:
	/**
	 * This function writes the uncompressed PRT file header, and records the file pointer position in order to later write the number of particles
	 * for the file. It expects 'm_layout' to not change afterwards, or else you are a bad human/android/robot.
	 */
	void write_header(){
		using namespace detail;

		// Write the main header data
		prt_header_v1 header;
		memset( &header, 0, sizeof(prt_header_v1) );

		header.magicNumber = prt_magic_number();
		header.headerLength = sizeof(prt_header_v1);
		strncpy(header.fmtIdentStr, prt_signature_string(), 32);
		header.version = 1;
		header.particleCount = -1;

		m_countLocation = ((char*)&header.particleCount - (char*)&header) + m_fout.tellp(); //This is where we need to seek to in order to write the particle count at the end.

		m_fout.write(reinterpret_cast<const char*>(&header), sizeof(prt_header_v1));

		// Write the reserved bytes
		prt_int32 reservedInt = 4;
		m_fout.write(reinterpret_cast<const char*>(&reservedInt), 4);

		// Write the channel map
		prt_channel_header_v1 prtChannel;

		prt_int32 channelCount = static_cast<prt_int32>( m_layout.num_channels() );
		prt_int32 channelHeaderItemSize = sizeof(prt_channel_header_v1);

		m_fout.write(reinterpret_cast<const char*>(&channelCount), 4);
		m_fout.write(reinterpret_cast<const char*>(&channelHeaderItemSize), 4);

		for( int i = 0; i < channelCount; ++i ) {
			memset( &prtChannel, 0, sizeof(prt_channel_header_v1) );

			std::string chName = m_layout.get_channel_name( static_cast<std::size_t>(i) );

			const detail::prt_channel& ch = m_layout.get_channel( chName );

			strncpy( prtChannel.channelName, chName.c_str(), 32 );
			prtChannel.channelArity = (prt_int32)ch.arity;
			prtChannel.channelType = (prt_int32)ch.type;
			prtChannel.channelOffset = (prt_int32)ch.offset;

			m_fout.write(reinterpret_cast<const char*>(&prtChannel), sizeof(prt_channel_header_v1));
		}
	}

	/**
	 * This function initializes the zlib decompression stream for the particle data portion of the PRT file.
	 */
	void init_zlib(){
		if(Z_OK != deflateInit( &m_zstream, Z_DEFAULT_COMPRESSION ) )
			throw std::runtime_error( "Unable to initialize a zlib deflate stream for output stream \"" + m_filePath + "\"." );

		if( m_bufferSize == 0 )
			m_bufferSize = (1 << 19);

		m_buffer = new char[m_bufferSize];

		m_zstream.avail_out = static_cast<unsigned int>( m_bufferSize );
		m_zstream.next_out = reinterpret_cast<unsigned char*>( m_buffer );
	}

	/**
	 * This helper function will write the compressed data stored in 'm_buffer' to disk.
	 */
	void flush(){
		std::size_t numOut = (m_bufferSize - m_zstream.avail_out);
		if( numOut > 0 ) {
			m_fout.write( m_buffer, numOut );
			m_zstream.avail_out = static_cast<unsigned int>( m_bufferSize );
			m_zstream.next_out = reinterpret_cast<unsigned char*>( m_buffer );
		}
	}

public:
	/**
	 * Default constructor. User must later call open().
	 */
	prt_ofstream(){
		m_buffer = NULL;
		m_bufferSize = 0;
		m_particleCount = 0;
		m_countLocation = 0;
		memset( &m_zstream, 0, sizeof(m_zstream) );
	}

	/**
	 * Can't have a constructor that opens the stream too, since we need to know the layout first!
	 */
	 //prt_ofstream( const std::string& file );

	virtual ~prt_ofstream(){
		close();
	}

	/**
	 * Opens the prt_ofstream to write to the specified file
	 * @param file Path to the file to write particles to
	 */
	void open( const std::string& file ){
		m_fout.open( file.c_str(), std::ios::out | std::ios::binary );
		if( m_fout.fail() )
			throw std::ios_base::failure( "Failed to open file \"" + file + "\" for writing" );

		m_filePath = file;
		m_fout.exceptions( std::ios::badbit|std::ios::failbit ); //We want an exception if writing anything fails.

		write_header();
		init_zlib();
	}

	/**
	 * Closes the stream, and deallocates any memory used for decompressing particles.
	 */
	void close(){
		if( m_buffer ){
			// Write out all the rest of the stream data, until we hit Z_STREAM_END
			while(Z_STREAM_END != deflate(&m_zstream, Z_FINISH))
				flush();
			flush();

			delete[] m_buffer;
			m_buffer = NULL;

			int ret = deflateEnd(&m_zstream);
			if( ret != Z_OK )
				throw std::runtime_error("deflateEnd() error for \"" + m_filePath + "\" was:" + zError(ret) );

			memset( &m_zstream, 0, sizeof(z_stream) );
		}

		//Seek back to the beginning of the file and write the particle count in the header region.
		if( m_fout.is_open() ){
			if( m_countLocation > 0 ){
				m_fout.seekp( m_countLocation, std::ios::beg );
				m_fout.write( reinterpret_cast<char*>( &m_particleCount ), 8 );
			}
			m_fout.close();
		}

		m_filePath.clear();
		m_layout.clear();

		m_bufferSize = 0;
		m_particleCount = 0;
		m_countLocation = 0;
	}

protected:
	/**
	 * Compresses a single particle into 'm_buffer' and flushes to disk if the buffer is full.
	 * @param data The data for the particle to write to disk.
	 */
	virtual void write_impl( const char* data ){
		++m_particleCount;

		m_zstream.avail_in = static_cast<unsigned>( m_layout.size() );
		m_zstream.next_in = reinterpret_cast<unsigned char*>( const_cast<char*>(data) );

		for(;;) {
			int ret = deflate(&m_zstream, Z_NO_FLUSH);
			if(ret == Z_STREAM_ERROR)
				throw std::runtime_error( "deflate() call writing to \"" + m_filePath + "\" failed:\n\t" + zError(ret) );

			if(m_zstream.avail_out == 0) {	//Did we fill the output buffer completely? If so flush and loop.
				flush();
			} else {
				break;
			}
		}
	}
};

}//namespace prtio