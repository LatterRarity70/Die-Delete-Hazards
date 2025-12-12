#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

#define MAX_TO_DESTROY Mod::get()->getSettingValue<int>("Objects to destroy at one attempt")

#include <Geode/modify/GameObject.hpp>
class $modify(GameObjectExt, GameObject) {
	void resetObject() {
		if (getUserObject("destroyed"_spr)) return;
		GameObject::resetObject();
	}
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(PlayLayerExt, PlayLayer) {
	struct Fields {
		CCLabelBMFont* m_remainObjectsStateLabel;
	};
	int& getRemainObjects() {
		return m_fields->m_remainObjectsStateLabel->m_nTag;
	}
	void updateRemainObjectsLabel() {
		if (!m_fields->m_remainObjectsStateLabel) return;
		m_fields->m_remainObjectsStateLabel->setString(fmt::format(
			"Hazards: {}/{}", getRemainObjects()
			, MAX_TO_DESTROY
		).c_str());
	}
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();

		m_fields->m_remainObjectsStateLabel = CCLabelBMFont::create("Objects remain to destroy: 0/0", "bigFont.fnt");
		if (m_uiLayer) if (m_uiLayer->m_pauseBtn) if (m_uiLayer->m_pauseBtn->getParent()) {
			auto menu = m_uiLayer->m_pauseBtn->getParent();
			m_fields->m_remainObjectsStateLabel->setScale(0.300);
			auto item = CCMenuItemExt::createSpriteExtra(
				m_fields->m_remainObjectsStateLabel, [uiLayer = Ref(m_uiLayer)](void*) {
					openInfoPopup(Mod::get());
					auto popup = CCScene::get()->getChildByType<FLAlertLayer>(0);
					if (popup) if (string::contains(popup->getID(), Mod::get()->getID())) {
						popup->removeFromParentAndCleanup(false);
						if (uiLayer) popup->m_scene = uiLayer;
						popup->show();
					}
				}
			);
			item->setAnchorPoint({ 0.5f, 1.5f });
			item->setPositionX(-menu->getPositionX() + menu->getContentSize().width / 2);
			menu->addChild(item);
		}
		m_fields->m_remainObjectsStateLabel->setTag(MAX_TO_DESTROY);
		updateRemainObjectsLabel();
	}
	virtual void destroyPlayer(PlayerObject* player, GameObject* object) {
		if (!m_fields->m_remainObjectsStateLabel) return PlayLayer::destroyPlayer(player, object);

		m_fields->m_remainObjectsStateLabel->getParent()->setVisible(
			Mod::get()->getSettingValue<bool>("Show state label")
		);

		if (!object or !player) return PlayLayer::destroyPlayer(player, object);
		if (object == m_anticheatSpike) return PlayLayer::destroyPlayer(player, object);

		auto& remain = getRemainObjects();

		if (!remain and (MAX_TO_DESTROY != -1)) PlayLayer::destroyPlayer(player, object);

		Ref obj = object;
		if (obj->m_objectType == GameObjectType::Solid) return;

		auto oldid = obj->m_objectID;
		obj->m_objectID = 143;
		obj->customSetup();
		obj->playDestroyObjectAnim(this);
		this->spawnParticle(
			"explodeEffectGrav.plist", obj->m_zOrder, //lol
			kCCPositionTypeRelative, obj->getRealPosition()
		);
		obj->destroyObject();
		obj->m_objectID = oldid;
		obj->customSetup();

		obj->setUserObject("destroyed"_spr, CCNode::create());

		if (remain) {
			FMODAudioEngine::get()->playEffect(
				MusicDownloadManager::sharedState()->pathForSFX(886)//"sfx/s886.ogg");
				, 1.0f + 0.5f * ((MAX_TO_DESTROY - remain) / (float)MAX_TO_DESTROY), 1.f, 0.5
			);
		}

		if (MAX_TO_DESTROY != -1) {
			remain--;
			remain = remain >= 0 ? remain : MAX_TO_DESTROY;
		}
		updateRemainObjectsLabel();
	}
};