#pragma once

#include <cerfcore/core/ui_text_service.h>
#include <cerffrontend/i18n/translation_catalog.h>

#include <map>
#include <string>

class UiLanguage : public cerfcore::UiTextService {
public:
    using cerfcore::UiTextService::UiTextService;
    void OnReady() override;
    const wchar_t* Text(std::string_view key) const;
    std::wstring Format(std::string_view key,
                        std::initializer_list<cerf::i18n::MessageArgument> arguments) const;
    std::string Translate(std::string_view key,
                          std::string_view fallback) const override;
    std::string FormatText(std::string_view key, std::string_view fallback,
                           cerfcore::UiTextArguments arguments) const override;
    const std::string& Language() const { return catalog_.Language(); }

private:
    cerf::i18n::TranslationCatalog catalog_;
    std::map<std::string, std::wstring, std::less<>> wide_messages_;
};
