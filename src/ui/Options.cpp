
#include <Windows.h>
#include <Spore/ModAPI.h>
#include "core/SdkCompat.hpp"
#include "core/Hooks.hpp"
#include "ui/Options.hpp"
#include <Spore/App/IMessageManager.h>
#include <Spore/App/cLocaleManager.h>
#include <Spore/UTFWin/IButton.h>
#include <Spore/UTFWin/IWindow.h>
#include <Spore/UTFWin/IWinProc.h>
#include <Spore/UTFWin/IWindowManager.h>
#include <cstring>
#include <cwchar>

namespace App {
IMessageManager* IMessageManager::Get() {
    return reinterpret_cast<IMessageManager* (*)()>(GetAddress(IMessageManager, Get))();
}
}

namespace TruelyOptions {
namespace {
using WindowManagerFn = UTFWin::IWindowManager* (*)();
using ButtonFactoryFn = UTFWin::IWindow*(__stdcall*)(void*, void*);
constexpr uint32_t WindowManagerAddress = 0x67ca60, ButtonFactoryAddress = 0x9670a0;
constexpr uint32_t StripID = 0x043de590, LastRowID = 0x4c0dd839;
constexpr uint32_t RowID = 0x7e1a7001, GroupID = 0x7e1a7002, OffCommand = 1, OnCommand = 3;
constexpr uint32_t LabelColor = 0xfff7fefe;
constexpr float RowPitch = 20, RowRaise = 2;
constexpr float LabelLeft = 2, LabelUp = 1;
constexpr uint32_t MediumCommand = 2, MediumShade = 0xff303030;
constexpr uint32_t LocaleManagerAddress = 0x67de00;
constexpr const char LocaleManagerBytes[] = "\xa1\xf8\xd8\x5f\x01\xc3";

struct Caption {
    const char* code;
    const char16_t* text;
};

constexpr Caption captions[] = {
    {"en-us", u"Water reflections"},
    {"en-gb", u"Water reflections"},
    {"cs-cz", u"Odrazy na vodě"},
    {"da-dk", u"Refleksioner i vand"},
    {"de-de", u"Wasserspiegelungen"},
    {"el-gr", u"Αντανακλάσεις νερού"},
    {"es-es", u"Reflejos del agua"},
    {"fi-fi", u"Veden heijastukset"},
    {"fr-fr", u"Reflets de l'eau"},
    {"hu-hu", u"Víztükröződés"},
    {"it-it", u"Riflessi dell'acqua"},
    {"ja-jp", u"水面の反射"},
    {"ko-kr", u"물 반사"},
    {"nl-nl", u"Waterreflecties"},
    {"no-no", u"Vannrefleksjoner"},
    {"pl-pl", u"Odbicia w wodzie"},
    {"pt-br", u"Reflexos na água"},
    {"pt-pt", u"Reflexos na água"},
    {"ru-ru", u"Отражения в воде"},
    {"sv-se", u"Vattenreflektioner"},
    {"th-th", u"การสะท้อนของน้ำ"},
    {"zh-cn", u"水面反射"},
    {"zh-tw", u"水面反射"},
};

uintptr_t base;
bool reflections = true;
wchar_t directory[512]{};
wchar_t file[512]{};

void* At(uint32_t address) {
    return reinterpret_cast<void*>(base + address - 0x400000);
}

bool Paths() {
    if (*file)
        return true;
    DWORD n = GetEnvironmentVariableW(L"APPDATA", directory, 450);
    if (!n || n >= 450)
        return false;
    wcscat_s(directory, L"\\TruelyNoLOD");
    wcscpy_s(file, directory);
    wcscat_s(file, L"\\settings.ini");
    return true;
}

bool Save() {
    if (!Paths() || (!CreateDirectoryW(directory, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS))
        return false;
    return WritePrivateProfileStringW(L"Graphics", L"WaterReflections", reflections ? L"1" : L"0", file) != FALSE;
}

void Load() {
    if (!Paths())
        return;
    wchar_t value[16]{};
    GetPrivateProfileStringW(L"Graphics", L"WaterReflections", L"1", value, 16, file);
    reflections = !(value[0] == L'0' && !value[1]);
}

void ApplyReflections() {
    if (void* props = *static_cast<void**>(At(AppPropertiesAddress)))
        reinterpret_cast<void(__thiscall*)(void*, uint32_t, bool)>(At(SetBoolAddress))(props, PlanetReflections,
                                                                                       reflections);
}

UTFWin::IWindow* Child(UTFWin::IWindowList_t::iterator it) {
    return static_cast<UTFWin::IWindow*>(&static_cast<UTFWin::Window_intrusive_list_node&>(*it));
}

UTFWin::IWindow* Find(UTFWin::IWindow* parent, uint32_t id, unsigned depth = 0) {
    if (!parent || depth > 24)
        return nullptr;
    if (parent->GetControlID() == id)
        return parent;
    for (auto it = parent->GetChildrenBegin(); it != parent->GetChildrenEnd(); ++it)
        if (auto* found = Find(Child(it), id, depth + 1))
            return found;
    return nullptr;
}

UTFWin::IWindow* Direct(UTFWin::IWindow* parent, uint32_t id) {
    for (auto it = parent->GetChildrenBegin(); it != parent->GetChildrenEnd(); ++it)
        if (Child(it)->GetControlID() == id)
            return Child(it);
    return nullptr;
}

UTFWin::IButton* RadioIn(UTFWin::IWindow* row, uint32_t command) {
    for (auto it = row->GetChildrenBegin(); it != row->GetChildrenEnd(); ++it) {
        auto* button = object_cast<UTFWin::IButton>(Child(it));
        if (button && button->GetButtonType() == UTFWin::ButtonTypes::Radio && Child(it)->GetCommandID() == command)
            return button;
    }
    return nullptr;
}

UTFWin::IWindow* LabelIn(UTFWin::IWindow* row) {
    for (auto it = row->GetChildrenBegin(); it != row->GetChildrenEnd(); ++it)
        if (!object_cast<UTFWin::IButton>(Child(it)) && Child(it)->GetCaption() && *Child(it)->GetCaption())
            return Child(it);
    return nullptr;
}

char Lower(char16_t c) {
    return c >= u'A' && c <= u'Z' ? char(c - u'A' + 'a') : c == u'_' ? '-' : char(c);
}

const char16_t* CaptionFor(const char16_t* code, unsigned length) {
    for (unsigned compare : {5u, 2u}) {
        if (length < compare)
            continue;
        for (const auto& caption : captions) {
            unsigned c = 0;
            while (c < compare && Lower(code[c]) == caption.code[c])
                ++c;
            if (c == compare)
                return caption.text;
        }
    }
    return nullptr;
}

const char16_t* ReflectionsCaption() {
    if (!memcmp(At(LocaleManagerAddress), LocaleManagerBytes, sizeof(LocaleManagerBytes) - 1))
        if (auto* manager = reinterpret_cast<App::cLocaleManager* (*)()>(At(LocaleManagerAddress))()) {
            const auto& code = manager->GetActiveLanguage();
            if (const char16_t* text = CaptionFor(code.c_str(), unsigned(code.size())))
                return text;
        }
    if (const wchar_t* found = wcsstr(GetCommandLineW(), L"-locale:")) {
        char16_t code[8]{};
        unsigned length = 0;
        for (const wchar_t* c = found + 8; *c && *c != L' ' && *c != L'"' && length < 7; ++c)
            code[length++] = char16_t(*c);
        if (const char16_t* text = CaptionFor(code, length))
            return text;
    }
    return captions[0].text;
}

UTFWin::IButton* NewButton() {
    auto* window = reinterpret_cast<ButtonFactoryFn>(At(ButtonFactoryAddress))(nullptr, nullptr);
    return window ? object_cast<UTFWin::IButton>(window) : nullptr;
}
}

namespace Window {
class Row final : public UTFWin::IWinProc {
public:
    UTFWin::IWindow* row = nullptr;
    UTFWin::IButton* off = nullptr;
    UTFWin::IButton* on = nullptr;
    UTFWin::IButton* medium = nullptr;

    int AddRef() override {
        return 2;
    }

    int Release() override {
        return 1;
    }

    void* Cast(uint32_t type) const override {
        if (type == UTFWin::IWinProc::TYPE || type == Object::TYPE)
            return const_cast<Row*>(this);
        return nullptr;
    }

    int GetPriority() const override {
        return 100;
    }

    int GetEventFlags() const override {
        return UTFWin::kEventFlagBasicInput | UTFWin::kEventFlagAdvanced;
    }

    void Show() {
        if (off)
            off->SetButtonStateFlag(UTFWin::kBtnStateSelected, !reflections);
        if (on)
            on->SetButtonStateFlag(UTFWin::kBtnStateSelected, reflections);
    }

    bool HandleUIMessage(UTFWin::IWindow*, const UTFWin::Message& msg) override {
        if (!msg.source || !(msg.IsType(UTFWin::kMsgButtonClick) || msg.IsType(UTFWin::kMsgButtonSelect)))
            return false;
        bool chosen;
        if (on && msg.source == on->ToWindow())
            chosen = true;
        else if (off && msg.source == off->ToWindow())
            chosen = false;
        else
            return false;
        if (chosen != reflections) {
            reflections = chosen;
            ApplyReflections();
            Save();
        }
        Show();
        return false;
    }

    static void Shift(UTFWin::IWindow* window, float dy) {
        const auto& a = window->GetArea();
        window->SetArea(Math::Rectangle(a.x1, a.y1 + dy, a.x2, a.y2 + dy));
    }

    bool Build(UTFWin::IWindow* last) {
        auto* parent = last->GetParent();
        auto* label = LabelIn(last);
        auto* nativeOff = RadioIn(last, OffCommand);
        auto* nativeOn = RadioIn(last, OnCommand);
        auto* nativeMedium = RadioIn(last, MediumCommand);
        if (!parent || !label || !nativeOff || !nativeOn || !nativeMedium)
            return false;
        UTFWin::IWindow* rows[16];
        unsigned count = 0;
        for (auto it = parent->GetChildrenBegin(); it != parent->GetChildrenEnd() && count < 16; ++it)
            if (RadioIn(Child(it), OffCommand))
                rows[count++] = Child(it);
        for (unsigned i = 1; i < count; ++i)
            for (unsigned k = i; k > 0 && rows[k]->GetArea().y1 < rows[k - 1]->GetArea().y1; --k) {
                auto* t = rows[k];
                rows[k] = rows[k - 1];
                rows[k - 1] = t;
            }
        const auto& frame = parent->GetArea();
        float top = rows[0]->GetArea().y1 - RowRaise;
        if (count < 2 || rows[count - 1] != last || top + RowPitch * (count + 1) > frame.GetHeight())
            return false;
        auto* box = NewButton();
        auto* text = NewButton();
        off = NewButton();
        on = NewButton();
        medium = NewButton();
        if (!box || !text || !off || !on || !medium)
            return false;
        for (unsigned i = 0; i < count; ++i) {
            const auto& a = rows[i]->GetArea();
            rows[i]->SetArea(Math::Rectangle(a.x1, top + RowPitch * i, a.x2, top + RowPitch * (i + 1)));
            for (auto it = rows[i]->GetChildrenBegin(); it != rows[i]->GetChildrenEnd(); ++it)
                Shift(Child(it), -1);
        }

        auto* closing = last->GetDrawable();
        if (!closing)
            last->SetDrawable(rows[count - 2]->GetDrawable());
        const auto& a = last->GetArea();
        row = box->ToWindow();
        row->SetDrawable(closing);
        row->SetCaption(u"");
        row->SetControlID(RowID);
        row->SetCommandID(0);
        row->SetArea(Math::Rectangle(a.x1, a.y2, a.x2, a.y2 + RowPitch));
        row->SetFillColor(Math::Color(0));
        row->SetIgnoreMouse(true);
        parent->AddWindow(row);

        auto* w = text->ToWindow();
        w->SetDrawable(nullptr);
        w->SetCaption(ReflectionsCaption());
        const auto& l = label->GetArea();
        w->SetArea(Math::Rectangle(l.x1 - LabelLeft, l.y1 - LabelUp, l.x2 - LabelLeft, l.y2 - LabelUp));
        w->SetTextFontID(label->GetTextFontID());
        w->SetFillColor(Math::Color(0));
        w->SetEnabled(false);
        w->SetIgnoreMouse(true);
        for (int i = 0; i < 8; ++i)
            text->SetCaptionColor(UTFWin::StateIndices(i), Math::Color(LabelColor));
        row->AddWindow(w);

        UTFWin::IButton* natives[2] = {nativeOff, nativeOn};
        UTFWin::IButton* ours[2] = {off, on};
        for (int i = 0; i < 2; ++i) {
            auto* b = ours[i];
            auto* bw = b->ToWindow();
            auto* nw = natives[i]->ToWindow();
            bw->SetCaption(u"");
            bw->SetControlID(0);
            bw->SetCommandID(i ? OnCommand : OffCommand);
            bw->SetArea(nw->GetArea());
            b->SetButtonType(UTFWin::ButtonTypes::Radio);
            b->SetButtonGroupID(GroupID);
            if (auto* drawable = natives[i]->GetButtonDrawable())
                b->SetButtonDrawable(drawable);
            else
                bw->SetDrawable(nw->GetDrawable());
            bw->SetVisible(true);
            bw->SetEnabled(true);
            bw->AddWinProc(this);
            row->AddWindow(bw);
        }
        auto* mw = medium->ToWindow();
        mw->SetCaption(u"");
        mw->SetControlID(0);
        mw->SetCommandID(0);
        mw->SetArea(nativeMedium->ToWindow()->GetArea());
        medium->SetButtonType(UTFWin::ButtonTypes::Radio);
        medium->SetButtonGroupID(0);
        if (auto* drawable = nativeMedium->GetButtonDrawable())
            medium->SetButtonDrawable(drawable);
        else
            mw->SetDrawable(nativeMedium->ToWindow()->GetDrawable());
        mw->SetShadeColor(Math::Color(MediumShade));
        mw->SetVisible(true);
        mw->SetEnabled(false);
        mw->SetIgnoreMouse(true);
        row->AddWindow(mw);
        Show();
        return true;
    }

    void Forget() {
        row = nullptr;
        off = on = medium = nullptr;
    }

    void Pump(UTFWin::IWindow* strip) {
        auto* last = strip ? Find(strip, LastRowID) : nullptr;
        if (!last || !last->GetParent()) {
            Forget();
            return;
        }
        if ((row && Direct(last->GetParent(), RowID) == row) || strip == failed)
            return;
        Forget();
        if (!Build(last)) {
            Forget();
            failed = strip;
        }
    }

private:
    UTFWin::IWindow* failed = nullptr;
};

Row reflectionsRow;

class Listener final : public App::IUnmanagedMessageListener {
    bool HandleMessage(uint32_t, void*) override {
        auto* manager = reinterpret_cast<WindowManagerFn>(At(WindowManagerAddress))();
        auto* strip = manager ? Find(manager->GetMainWindow(), StripID) : nullptr;
        if (!strip)
            return false;
        reflectionsRow.Pump(strip);
        return false;
    }
};

Listener listener;
}

void Start(uintptr_t gameBase) {
    base = gameBase;
    Load();
    ApplyReflections();
    if (auto* messages = App::IMessageManager::Get())
        messages->AddUnmanagedListener(&Window::listener, App::kMsgAppUpdate);
}

bool Reflections() {
    return reflections;
}
}

