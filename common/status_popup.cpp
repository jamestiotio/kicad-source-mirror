/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * Transient mouse following popup window implementation.
 */

#include <wx/settings.h>
#include <math/vector2wx.h>
#include <status_popup.h>
#include <eda_draw_frame.h>
#include <bitmaps.h>

STATUS_POPUP::STATUS_POPUP( wxWindow* aParent ) :
        wxPopupWindow( aParent ),
        m_expireTimer( this )
{
    SetDoubleBuffered( true );

    m_panel = new wxPanel( this, wxID_ANY );
    m_topSizer = new wxBoxSizer( wxHORIZONTAL );
    m_panel->SetSizer( m_topSizer );
    m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    Connect( wxEVT_TIMER, wxTimerEventHandler( STATUS_POPUP::onExpire ), nullptr, this );

#ifdef __WXOSX_MAC__
    // Key events from popups don't get put through the wxWidgets event system on OSX,
    // so we have to fall back to the CHAR_HOOK to forward hotkeys from the popup to
    // the canvas / frame.
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( STATUS_POPUP::onCharHook ), nullptr, this );
#endif
}


void STATUS_POPUP::onCharHook( wxKeyEvent& aEvent )
{
    // Key events from the status popup don't get put through the wxWidgets event system on
    // OSX, so we have to fall back to the CHAR_HOOK to forward hotkeys from the popup to
    // the canvas / frame.
    aEvent.SetEventType( wxEVT_CHAR );

    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( GetParent() );

    if( frame )
        frame->GetCanvas()->OnEvent( aEvent );
    else
        GetParent()->GetEventHandler()->ProcessEvent( aEvent );
}


void STATUS_POPUP::Popup( wxWindow* )
{
    Show( true );
    Raise();
}


void STATUS_POPUP::PopupFor( int aMsecs )
{
    Popup();
    Expire( aMsecs );
}


void STATUS_POPUP::Move( const VECTOR2I& aWhere )
{
    SetPosition( ToWxPoint( aWhere ) );
}


void STATUS_POPUP::Move( const wxPoint& aWhere )
{
    SetPosition( aWhere );
}


void STATUS_POPUP::Expire( int aMsecs )
{
    m_expireTimer.StartOnce( aMsecs );
}


void STATUS_POPUP::updateSize()
{
    m_topSizer->Fit( m_panel );
    SetClientSize( m_panel->GetSize() );
}


void STATUS_POPUP::onExpire( wxTimerEvent& aEvent )
{
    Hide();
}


STATUS_TEXT_POPUP::STATUS_TEXT_POPUP( wxWindow* aParent ) :
    STATUS_POPUP( aParent )
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_panel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );

    m_statusLine = new wxStaticText( m_panel, wxID_ANY, wxEmptyString ) ;
    m_topSizer->Add( m_statusLine, 1, wxALL | wxEXPAND, 5 );
}


void STATUS_TEXT_POPUP::SetText( const wxString& aText )
{
    m_statusLine->SetLabel( aText );
    updateSize();
}


void STATUS_TEXT_POPUP::SetTextColor( const wxColour& aColor )
{
    m_statusLine->SetForegroundColour( aColor );
}


STATUS_MIN_MAX_POPUP::STATUS_MIN_MAX_POPUP( EDA_DRAW_FRAME* aFrame ) :
        STATUS_POPUP( aFrame ),
        m_frame( aFrame ),
        m_min( 0.0 ),
        m_max( 0.0 )
{
    m_icon = new wxStaticBitmap( m_panel, wxID_ANY, KiBitmap( BITMAPS::checked_ok ),
                                 wxDefaultPosition, wxSize( 12, 12 ) );

    m_currentLabel = new wxStaticText( m_panel, wxID_ANY, _( "current" ) );
    wxStaticText* minLabel = new wxStaticText( m_panel, wxID_ANY, _( "min" ) );
    wxStaticText* maxLabel = new wxStaticText( m_panel, wxID_ANY, _( "max" ) );

    wxFont infoFont = KIUI::GetStatusFont( this );
    m_currentLabel->SetFont( infoFont );
    minLabel->SetFont( infoFont );
    maxLabel->SetFont( infoFont );

    m_currentText = new wxStaticText( m_panel, wxID_ANY, wxEmptyString );
    m_minText = new wxStaticText( m_panel, wxID_ANY, wxEmptyString );
    m_maxText = new wxStaticText( m_panel, wxID_ANY, wxEmptyString );

    wxBoxSizer* currentSizer = new wxBoxSizer( wxVERTICAL );
    currentSizer->Add( m_currentLabel, 0, 0, 5 );
    currentSizer->Add( m_currentText, 0, 0, 5 );

    wxBoxSizer* minSizer = new wxBoxSizer( wxVERTICAL );
    minSizer->Add( minLabel, 0, 0, 5 );
    minSizer->Add( m_minText, 0, 0, 5 );

    wxBoxSizer* maxSizer = new wxBoxSizer( wxVERTICAL );
    maxSizer->Add( maxLabel, 0, 0, 5 );
    maxSizer->Add( m_maxText, 0, 0, 5 );

    m_topSizer->Add( currentSizer, 0, wxLEFT | wxRIGHT, 3 );
    m_topSizer->Add( m_icon, 0, wxALL | wxALIGN_BOTTOM | wxRESERVE_SPACE_EVEN_IF_HIDDEN, 1 );
    m_topSizer->Add( minSizer, 0, wxLEFT | wxRIGHT, 3 );
    m_topSizer->Add( maxSizer, 0, wxLEFT | wxRIGHT, 3 );
}


void STATUS_MIN_MAX_POPUP::SetMinMax( double aMin, double aMax )
{
    m_min = aMin;
    m_minText->SetLabel( m_frame->MessageTextFromValue( m_min, false ) );
    m_max = aMax;
    m_maxText->SetLabel( m_frame->MessageTextFromValue( m_max, false ) );
}


void STATUS_MIN_MAX_POPUP::SetCurrent( double aCurrent, const wxString& aLabel )
{
    m_currentLabel->SetLabel( aLabel );
    m_currentText->SetLabel( m_frame->MessageTextFromValue( aCurrent ) );
    m_icon->Show( aCurrent >= m_min && aCurrent <= m_max );

    wxColour normal = wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT );

    // Determine the background color first and choose a contrasting value
    COLOR4D bg = GetBackgroundColour();
    COLOR4D red;
    double  bg_h, bg_s, bg_l;
    bg.ToHSL( bg_h, bg_s, bg_l );
    red.FromHSL( 0, 1.0, bg_l < 0.5 ? 0.7 : 0.3 );

    m_minText->SetForegroundColour( aCurrent < m_min ? red.ToColour() : normal );
    m_maxText->SetForegroundColour( aCurrent > m_max ? red.ToColour() : normal );

    m_topSizer->Layout();
    updateSize();

    Refresh();
    Update();
}