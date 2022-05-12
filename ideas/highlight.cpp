#include "parser.hpp"
#include <wx/wx.h>
#include <wx/stc/stc.h>

namespace tcc
{
    struct StyleText
    {
        enum class Style
        {
            DEFAULT = 0, COMMENT = 4, DECLARE, FUNCTION, CONSTANT, 
            VARIABLE, NUMBER, UNDEFINED, SYMBOL, ERROR, BRACKETS
        };

        int start = 0;
        int length = 0;
        Style style = Style::DEFAULT;
    };

    struct Highlight : public Parser
    {
        std::vector<StyleText> styles;
        std::vector<int> stack;

        struct Pair
        {
            int a, b;
        };
        std::vector<Pair> pairs;

        void actAdvance()
        {
            StyleText s;
            s.start = lexer.lexeme - lexer.source;
            s.length = lexer.length;

            if((int)lexer.type < 256)
            {
                int pos = (int)(lexer.lexeme - lexer.source);
                s.style = StyleText::Style::SYMBOL;
                switch((int)lexer.type)
                {
                    case '(':
                    case '[':
                        stack.push_back(pairs.size());
                        pairs.push_back({pos, 0});
                        break;
                    case ')':
                    case ']':
                        if(stack.size())
                        {
                            pairs[stack.back()].b = pos;
                            stack.pop_back();
                        }
                        break;
                }
            }
            else switch(lexer.type)
            {
                case TokenType::UNDEFINED: s.style = StyleText::Style::UNDEFINED; break;
                case TokenType::EOI: s.style = StyleText::Style::SYMBOL; break;
                case TokenType::DECLARE: s.style = StyleText::Style::DECLARE; break;
                case TokenType::CONSTANT: s.style = StyleText::Style::CONSTANT; break;
                case TokenType::VARIABLE: s.style = StyleText::Style::VARIABLE; break;
                case TokenType::NUMBER: s.style = StyleText::Style::NUMBER; break;
                case TokenType::FUNCTION: s.style = StyleText::Style::FUNCTION; break;
                case TokenType::COMMENT: s.style = StyleText::Style::COMMENT; break;
            }
            styles.push_back(s);
        }
    };

    class SourceEditor : public wxFrame
    {
    public:
        SourceEditor(const wxString& title);
        wxStyledTextCtrl *ctrl;
        std::vector<Highlight::Pair> pairs;

        void UpdateUI(wxStyledTextEvent& event);
    };

    class MyApp : public wxApp
    {
    public:
        virtual bool OnInit();
    };

    SourceEditor::SourceEditor(const wxString& title)
        : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(512, 512))
    {
        Centre();
        ctrl = new wxStyledTextCtrl(this);
        //ctrl->SetLexer(wxSTC_LEX_CONTAINER);
        ctrl->Bind(wxEVT_STC_UPDATEUI, &SourceEditor::UpdateUI, this);

        wxFont italic;
        italic.SetFaceName("C059");
        italic.SetStyle(wxFONTSTYLE_ITALIC);

        wxFont bold;
        bold.SetFaceName("FreeMono");
        bold.SetWeight(wxFONTWEIGHT_BOLD);

        wxFont bolditalic;
        bolditalic.SetFaceName("FreeMono");
        bolditalic.SetWeight(wxFONTWEIGHT_BOLD);
        bolditalic.SetStyle(wxFONTSTYLE_ITALIC);

        wxFont normal;
        normal.SetFaceName("FreeMono");

        #define STYLE(x,y,z) ctrl->StyleSet##x((int)StyleText::Style::y, z)

            STYLE(Foreground, DEFAULT,  wxColor(0, 0, 0));
            STYLE(Foreground, SYMBOL,   wxColor(0, 0, 0));
            STYLE(Foreground, COMMENT,  wxColor(0, 100, 0));
            STYLE(Foreground, DECLARE,  wxColor(50, 50, 200));
            STYLE(Foreground, FUNCTION, wxColor(50, 50, 50));
            STYLE(Foreground, CONSTANT, wxColor(100, 120, 150));
            STYLE(Foreground, VARIABLE, wxColor(242, 150, 11));
            STYLE(Foreground, NUMBER,   wxColor(100, 100, 255));
            STYLE(Foreground, UNDEFINED,wxColor(242, 150, 11));
            STYLE(Background, ERROR,    wxColor(255, 0, 0));
            STYLE(Background, BRACKETS, wxColor(200, 200, 255));

            STYLE(Font, DEFAULT, normal);
            STYLE(Font, SYMBOL, bold);
            STYLE(Font, COMMENT, bolditalic);
            STYLE(Font, DECLARE, bold);
            STYLE(Font, FUNCTION, italic);
            STYLE(Font, CONSTANT, italic);
            STYLE(Font, VARIABLE, italic);
            STYLE(Font, UNDEFINED, italic);
            STYLE(Font, ERROR, bold);
            STYLE(Font, BRACKETS, bold);

        #undef STYLE

        wxColor col = wxColor(255, 0, 0);
        ctrl->MarkerDefine(1, wxSTC_MARK_CIRCLE, wxNullColour, col);
    }

    void SourceEditor::UpdateUI(wxStyledTextEvent& event)
    {
        wxString text = ctrl->GetText();
        size_t length = text.length();

        if(length == 0) return;

        bool updated = false;
        bool error = false;

        if(event.GetUpdated() & wxSTC_UPDATE_CONTENT)
        {
            ctrl->MarkerDeleteAll(1);
            ctrl->StartStyling(0);
            ctrl->SetStyling(length, 0);

            Highlight hl;

            try
            {
                hl.parseProgram(text.c_str());
                pairs = hl.pairs;
            } catch(const char *msg)
            {
                ctrl->StartStyling(hl.lexer.lexeme - hl.lexer.source);
                ctrl->SetStyling(hl.lexer.length, (int)StyleText::Style::ERROR);
                ctrl->MarkerAdd(hl.lexer.lineno, 1);
                pairs.clear();
                error = true;
            }

            for(StyleText &s : hl.styles)
            {
                ctrl->StartStyling(s.start);
                ctrl->SetStyling(s.length, (int)s.style);
            }

            if(error) return;

            updated = true;
        }
        
        if(updated || (event.GetUpdated() & wxSTC_UPDATE_SELECTION))
        {
            if(pairs.size())
            {
                int pos = ctrl->GetCurrentPos();
                int p = -1;

                for(int i = 0; i < (int)pairs.size(); i++)
                    if(pairs[i].a <= pos && pairs[i].b >= pos) 
                        p = i;

                for(int i = 0; i < (int)pairs.size(); i++)
                {
                    int style = i == p
                        ? (int)StyleText::Style::BRACKETS 
                        : (int)StyleText::Style::SYMBOL;
                    ctrl->StartStyling(pairs[i].a);
                    ctrl->SetStyling(1, style);
                    ctrl->StartStyling(pairs[i].b);
                    ctrl->SetStyling(1, style);
                }
            }
        }
    }

    bool MyApp::OnInit()
    {
        SourceEditor *simple = new SourceEditor(wxT("Syntax Highlight"));
        simple->Show(true);
        return true;
    }

}

IMPLEMENT_APP(tcc::MyApp)