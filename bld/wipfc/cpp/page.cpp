/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2009-2018 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  A page or panel data
*
****************************************************************************/


#include "wipfc.hpp"
#include "page.hpp"
#include "cell.hpp"
#include "document.hpp"
#include "errors.hpp"
#include "hn.hpp"

void Page::buildTOC()
{
    ( *( elements.begin() ))->buildTOC( this );
}
/***************************************************************************/
void Page::buildLocalDictionary()
{
    bool autoSpacing( true );
    currentCell = new Cell( document->maxLocalDictionarySize() );
    document->addCell( currentCell );
    cells.push_back( static_cast< word >( currentCell->index() ) );
    ++toc.cellCount;
    for( ElementIter itr = elements.begin(); itr != elements.end(); ++itr ) {
        std::pair< bool, bool > flags( ( *itr )->buildLocalDict( this ) );
        if( flags.first ) {
            currentCell = new Cell( document->maxLocalDictionarySize() );
            document->addCell( currentCell );
            cells.push_back( static_cast< word >( currentCell->index() ) );
            ++toc.cellCount;
            if( !autoSpacing )          //autoSpacing can't cross a cell boundry
                currentCell->addByte( 0xFC );   //so turn it off so we can turn
            flags = ( *itr )->buildLocalDict( this ); //it back on later
        }
        if( flags.second )
            autoSpacing = !autoSpacing;
        currentCell->addElement( *itr );
    }
}
/***************************************************************************/
bool Page::addWord( GlobalDictionaryWord* Word )
{
    if( Word ) {    //can be 0 for unrecognized entity references
        if( currentCell->dictFull() )
            return( true );
        currentCell->addWord( Word->index() );
        Word->onPage( idx );
    }
    return( false );
}
/***************************************************************************/
//Write a TOC entry
STD1::uint32_t Page::write( std::FILE* out )
{
    std::size_t tocsize( sizeof( TocEntry ) + toc.cellCount * sizeof( word ) );
    if( toc.extended ) {
        tocsize += sizeof( ExtTocEntry );
        if( etoc.setPos )
            tocsize += sizeof( PageOrigin );
        if( etoc.setSize )
            tocsize += sizeof( PageSize );
        if( etoc.setStyle )
            tocsize += sizeof( PageStyle );
        if( etoc.setGroup )
            tocsize += sizeof ( PageGroup );
        if( etoc.setCtrl ) {
            tocsize += sizeof( PageControls );
        }
    }
    if( tocsize + title.size() > 255 ) {
        Hn* hn( static_cast< Hn* >( *( elements.begin() ) ) );
        hn->printError( ERR2_TEXTTOOLONG );
        title.erase( 255 - tocsize );  //write only part of title
    }
    tocsize += title.size();
    toc.size = static_cast< byte >( tocsize );
    dword pos = toc.write( out );
    if( toc.extended ) {
        etoc.write( out );
        if( etoc.setPos )
            origin.write( out );
        if( etoc.setSize )
            size.write (out );
        if( etoc.setStyle )
            style.write( out );
        if( etoc.setGroup )
            group.write( out );
        if( etoc.setCtrl ) {
            controls.write( out );
        }
    }
    if( std::fwrite( &cells[0], sizeof( word ), cells.size(), out ) != cells.size() )
        throw FatalError( ERR_WRITE );
    if( !title.empty() ){
        if( std::fwrite( title.c_str(), sizeof( byte ), title.size(), out ) != title.size() )
            throw FatalError( ERR_WRITE );
        }
    }
    return( pos );
}
/***************************************************************************/
// byte size
// word parent_toc_index
// word child_toc_index
STD1::uint32_t Page::writeChildren( std::FILE* out ) const
{
    dword bytes = 0;
    if( !children.empty() ) {
        byte size_u8 = 3 + static_cast< byte >( children.size() * sizeof( word ) );
        if( std::fputc( size_u8, out ) == EOF )
            throw FatalError( ERR_WRITE );
        ++bytes;
        if( std::fwrite( &idx, sizeof( word ), 1, out ) != 1 )
            throw FatalError( ERR_WRITE );
        bytes += sizeof( word );
        if( std::fwrite( &children[0], sizeof( word ), children.size(), out ) != children.size() )
            throw FatalError( ERR_WRITE );
        bytes += sizeof( word ) * children.size();
    }
    return( bytes );
}
