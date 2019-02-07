#include <sstream>

#include <windows.h>

#include "ObjectExplorer.hpp"


ObjectExplorer::ObjectExplorer()
{
    m_objectAddress.reserve(256);
}

void ObjectExplorer::onDrawUI() {
    ImGui::SetNextTreeNodeOpen(false, ImGuiCond_::ImGuiCond_Once);

    if (!ImGui::CollapsingHeader(getName().data())) {
        return;
    }

    ImGui::InputText("REObject Address", m_objectAddress.data(), 16, ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsHexadecimal);

    if (m_objectAddress[0] != 0) {
        handleAddress(std::stoull(m_objectAddress, nullptr, 16));
    }
}

void ObjectExplorer::handleAddress(Address address, int32_t offset) {
    if (!isManagedObject(address)) {
        return;
    }

    auto object = address.as<REManagedObject*>();

    bool madeNode = false;
    auto isGameObject = utility::REManagedObject::isA(object, "via.GameObject");

    if (offset != -1) {
        ImGui::SetNextTreeNodeOpen(false, ImGuiCond_::ImGuiCond_Once);

        if (isGameObject) {
            madeNode = ImGui::TreeNode((uint8_t*)object + offset, "0x%X: %s", offset, utility::REString::getString(address.as<REGameObject*>()->name).c_str());
        }
        else {
            madeNode = ImGui::TreeNode((uint8_t*)object + offset, "0x%X: %s", offset, object->info->classInfo->type->name);
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::Selectable("Copy")) {
                std::stringstream ss;
                ss << std::hex << (uintptr_t)object;

                ImGui::SetClipboardText(ss.str().c_str());
            }

            ImGui::EndPopup();
        }
    }

    if (madeNode || offset == -1) {
        if (isGameObject) {
            handleGameObject(address.as<REGameObject*>());
        }

        if (utility::REManagedObject::isA(object, "via.Component")) {
            handleComponent(address.as<REComponent*>());
        }

        handleType(utility::REManagedObject::getType(object));

        if (ImGui::TreeNode(object, "AutoGenerated Types")) {
            auto typeInfo = object->info->classInfo->type;

            for (auto i = (int32_t)sizeof(void*); i < typeInfo->size - sizeof(void*); i += sizeof(void*)) {
                auto ptr = Address(object).get(i).to<REManagedObject*>();

                handleAddress(ptr, i);
            }

            ImGui::TreePop();
        }
    }

    if (madeNode && offset != -1) {
        ImGui::TreePop();
    }
}

void ObjectExplorer::handleGameObject(REGameObject* gameObject) {
    ImGui::Text("Name: %s", utility::REString::getString(gameObject->name).c_str());
    makeTreeOffset(gameObject, offsetof(REGameObject, transform), "Transform");
    makeTreeOffset(gameObject, offsetof(REGameObject, folder), "Folder");
}

void ObjectExplorer::handleComponent(REComponent* component) {
    makeTreeOffset(component, offsetof(REComponent, ownerGameObject), "Owner");
    makeTreeOffset(component, offsetof(REComponent, idkComponent), "IdkComponent");
    makeTreeOffset(component, offsetof(REComponent, prevComponent), "PrevComponent");
    makeTreeOffset(component, offsetof(REComponent, nextComponent), "NextComponent");
}

void ObjectExplorer::handleTransform(RETransform* transform) {

}

void ObjectExplorer::handleType(REType* t) {
    if (t == nullptr) {
        return;
    }

    auto count = 0;

    for (auto typeInfo = t; typeInfo != nullptr; typeInfo = typeInfo->super) {
        auto name = typeInfo->name;

        if (name == nullptr) {
            continue;
        }

        if (!ImGui::TreeNode(name)) {
            break;
        }

        ImGui::Text("Size: 0x%X", typeInfo->size);

        ++count;

        if (typeInfo->fields != nullptr && typeInfo->fields->variables != nullptr && typeInfo->fields->variables->data != nullptr) {
            auto descriptors = typeInfo->fields->variables->data->descriptors;

            if (ImGui::TreeNode("Fields", "Fields: %i", typeInfo->fields->variables->num)) {
                for (auto i = descriptors; i != descriptors + typeInfo->fields->variables->num; ++i) {
                    auto variable = *i;

                    if (variable == nullptr) {
                        continue;
                    }

                    ImGui::Text(variable->name);
                }

                ImGui::TreePop();
            }
        }
    }

    for (auto i = 0; i < count; ++i) {
        ImGui::TreePop();
    }

}

void ObjectExplorer::makeTreeOffset(REManagedObject* object, uint32_t offset, std::string_view name) {
    auto ptr = Address(object).get(offset).to<void*>();

    if (ptr == nullptr) {
        return;
    }

    auto nameString = std::string{ name };

    if (ImGui::TreeNode((uint8_t*)object + offset, "0x%X: %s", offset, name.data())) {
        handleAddress(ptr);
        ImGui::TreePop();
    }
}

bool ObjectExplorer::isManagedObject(Address address) const {
    return utility::REManagedObject::isManagedObject(address);
}
