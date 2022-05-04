//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <memory>

#include <System.Win.ComObj.hpp>

#include "XmlLite.h"
#include "Unit1.h"

using std::make_unique;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma comment( lib, "xmllite" )

TForm1 *Form1;
//---------------------------------------------------------------------------

__fastcall TForm1::TForm1( TComponent* Owner )
    : TForm( Owner )
{
}
//---------------------------------------------------------------------------

//tracer
class TMyFileStream : public TFileStream
{
public:
    __fastcall TMyFileStream(const System::UnicodeString AFileName, System::Word Mode)
      : TFileStream(AFileName, Mode)
    {
        Form1->Memo1->Lines->
        Append( __PRETTY_FUNCTION__ );
    }
    __fastcall TMyFileStream(const System::UnicodeString AFileName, System::Word Mode, unsigned Rights)
      : TFileStream(AFileName, Mode, Rights)
    {
        Form1->Memo1->Lines->Append( __PRETTY_FUNCTION__ );
    }
    __fastcall virtual ~TMyFileStream(void)
    {
        Form1->Memo1->Lines->Append( __PRETTY_FUNCTION__ );
    }
};

//tracer
class TMyStreamAdapter : public TStreamAdapter {
public:
    __fastcall TMyStreamAdapter(TStream* Stream, TStreamOwnership Ownership)
        : TStreamAdapter( Stream, Ownership )
    {
        Form1->Memo1->Lines->Append( __PRETTY_FUNCTION__ );
    }
    __fastcall ~TMyStreamAdapter() {
        Form1->Memo1->Lines->Append( __PRETTY_FUNCTION__ );
    }
};

String XmlNodeTypeToString( XmlNodeType Value )
{
    switch ( Value ) {
        case XmlNodeType_None:                  return _T( "None" );
        case XmlNodeType_Element:               return _T( "Element" );
        case XmlNodeType_Attribute:             return _T( "Attribute" );
        case XmlNodeType_Text:                  return _T( "Text" );
        case XmlNodeType_CDATA:                 return _T( "CDATA" );
        case XmlNodeType_ProcessingInstruction: return _T( "Processing Instruction" );
        case XmlNodeType_Comment:               return _T( "Comment" );
        case XmlNodeType_DocumentType:          return _T( "Document Type" );
        case XmlNodeType_Whitespace:            return _T( "Whitespace" );
        case XmlNodeType_EndElement:            return _T( "End Element" );
        case XmlNodeType_XmlDeclaration:        return _T( "Xml Declaration" );
        default:                                return _T( "Unknown" );
    }
}

void __fastcall TForm1::Button1Click(TObject *Sender)
{
    Memo1->Clear();

    typedef  DelphiInterface<IXmlReader>  _di_IXmlReader;
    typedef  DelphiInterface<IXmlReaderInput>  _di_IXmlReaderInput;

    const String FileName = _T( "../../Resource/simple.xml" );

    auto VCLStream =
        make_unique<TMyFileStream>( FileName, fmOpenRead | fmShareDenyNone );

    TStreamAdapter* const VCLStreamAdapter(
        new TMyStreamAdapter(
            VCLStream.get(),
            soReference
        )
    );

    _di_IStream Stream = *VCLStreamAdapter;

    _di_IXmlReader Reader;

    OleCheck( ::CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>( &Reader ), 0 ) );

#if 0
    // To use the stream directly
    Reader->SetInput( Stream );
#else
    // To override the encoding
    _di_IXmlReaderInput Input;
    OleCheck(
        ::CreateXmlReaderInputWithEncodingName(
            Stream,
            nullptr,  // default allocator
            L"UTF-8", // See: http://msdn.microsoft.com/en-us/ms752827.aspx
            TRUE,     // See: http://msdn.microsoft.com/en-us/library/ms752899%28v=vs.85%29.aspx
            nullptr,  // base URI
            &Input
        )
    );
    OleCheck( Reader->SetInput( Input ) );
#endif

    auto SB = make_unique<TStringBuilder>();

    HRESULT result = S_OK;
    XmlNodeType nodeType = XmlNodeType_None;
    while ( S_OK == ( result = Reader->Read( &nodeType ) ) ) {
        if ( nodeType != XmlNodeType_Whitespace ) {
            SB->AppendFormat(
                _T( "\r\nNodeType: %s\r\n" ),
                ARRAYOFCONST( ( XmlNodeTypeToString( nodeType ) ) )
            );

            LPCWSTR Text;
            UINT AttrCount;

            switch ( nodeType ) {
                case XmlNodeType_Element:
                    OleCheck( Reader->GetAttributeCount( &AttrCount ) );
                    OleCheck( Reader->GetLocalName( &Text, 0 ) );
                    SB->AppendFormat(
                        _T( "  LocalName: %s, AttributeCount=%d\r\n" ),
                        ARRAYOFCONST( ( Text, AttrCount ) )
                    );
                    if ( AttrCount ) {
                        size_t AttrIdx = 0;
                        for ( HRESULT Res = Reader->MoveToFirstAttribute();
                              S_OK == Res;
                              Res = Reader->MoveToNextAttribute() )
                        {
                            // Get attribute-specific info
                            OleCheck( Reader->GetLocalName( &Text, 0 ) );
                            SB->AppendFormat(
                                _T( "  %3d: Attr[\"%s\"]" ),
                                ARRAYOFCONST( ( AttrIdx, Text ) )
                            );
                            OleCheck( Reader->GetValue( &Text, 0 ) );
                            SB->AppendFormat(
                                _T( "=%s" ),
                                ARRAYOFCONST( ( Text ) )
                            );
                            if ( --AttrCount ) {
                                SB->AppendLine();
                            }
                            ++AttrIdx;
                        }
                        SB->AppendLine();
                    }
                    // http://msdn.microsoft.com/en-us/magazine/cc163436.aspx
                    // When  you call  the IXmlReader's  Read method,  it
                    // automatically  stores  any node  attributes  in an
                    // internal collection. This  allows you to  move the
                    // reader to a specific  attribute by name using  the
                    // MoveToAttributeByName  method.   However,  it   is
                    // usually more efficient to enumerate the attributes
                    // and  store  them in  an  application-specific data
                    // structure. Note  that you  can also  determine the
                    // number of attributes in the current node using the
                    // GetAttributeCount method.
                    // XmlLite Samples: http://goo.gl/hWm9ZO
                    // See also: http://goo.gl/LbqelP
                    break;
                case XmlNodeType_Text:
                    // It's also possible to use IXmlReader::ReadValueChunk()
                    OleCheck( Reader->GetValue( &Text, 0 ) );
                    SB->AppendFormat( _T( "  Value: %s\r\n" ), ARRAYOFCONST( ( Text ) ) );
                    break;
                case XmlNodeType_EndElement:
                    OleCheck( Reader->GetLocalName( &Text, 0 ) );
                    SB->AppendFormat(
                        _T( "  LocalName: %s\r\n" ),
                        ARRAYOFCONST( ( Text ) )
                    );
                    break;
                default:
                    break;
            }
        }
    }

    Memo1->Lines->Add( SB->ToString() );
}
//---------------------------------------------------------------------------


