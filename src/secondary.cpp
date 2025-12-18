#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

#define MAX_TO_DESTROY Mod::get()->getSettingValue<int>("Objects to destroy at one attempt")
#define ADD_OBJECT_EXPLOSION Mod::get()->getSettingValue<bool>("Add Object Explosion")
#define EXPLOSION_SPRITES_LIFETIME Mod::get()->getSettingValue<float>("Explosion Sprites Lifetime") //Explosion Sprites Lifetime
#define EXPLOSION_SPRITES_COUNT_X Mod::get()->getSettingValue<int>("Explosion Sprites Count X") //Explosion Sprites Count X
#define EXPLOSION_SPRITES_COUNT_Y Mod::get()->getSettingValue<int>("Explosion Sprites Count Y") //Explosion Sprites Count Y
#define EXPLOSION_WITHOUT_PARTICLES Mod::get()->getSettingValue<bool>("Explosion Without Particles") //Explosion Without Particles
#define SPAWN_PARTICLE Mod::get()->getSettingValue<std::string>("Spawn Particle") // Spawn Particle
#define DESTROY_SFX Mod::get()->getSettingValue<int>("Destroy SFX") // Remain Ones Destroy SFX

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

	void createObjectExplosion(PlayerObject* player, GameObject* obj) {
		if (!obj) return;

		CCSize size = obj->getContentSize();
		CCRenderTexture* texture = CCRenderTexture::create(size.width, size.height);

		texture->begin();

		CCPoint og = obj->getPosition();

		obj->setFlipX(false);
		obj->setFlipY(false);
		obj->setRotation(0);
		obj->setPosition(
			-obj->getPosition() + m_objectLayer->convertToNodeSpace({ 0, 0 }) 
			+ size + m_objectLayer->getPosition()
		);
		obj->visit();
		obj->setPosition(og);

		texture->end();

		ExplodeItemNode* explosion = ExplodeItemNode::create(texture);
		explosion->setPosition(obj->getPosition());
		m_objectLayer->addChild(explosion, 101);

		explosion->createSprites(
			1 + rand() % EXPLOSION_SPRITES_COUNT_X, 
			1 + rand() % EXPLOSION_SPRITES_COUNT_Y,
			player->m_playerSpeed + player->getCurrentXVelocity(), 5,
			player->m_playerSpeed + player->m_yVelocity, 3.f,
			EXPLOSION_SPRITES_LIFETIME, EXPLOSION_SPRITES_LIFETIME / 2,
			{ 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f }, 
			EXPLOSION_WITHOUT_PARTICLES
		);
	};

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

		if (ADD_OBJECT_EXPLOSION) createObjectExplosion(player, object);
		this->spawnParticle(
			SPAWN_PARTICLE.c_str(), obj->m_zOrder, //lol
			kCCPositionTypeRelative, obj->getRealPosition()
		);

		auto oldid = obj->m_objectID;
		obj->m_objectID = 143;
		obj->customSetup();
		obj->playDestroyObjectAnim(this);
		obj->destroyObject();
		obj->m_objectID = oldid;
		obj->customSetup();

		obj->setUserObject("destroyed"_spr, CCNode::create());

		FMODAudioEngine::get()->playEffect(
			MusicDownloadManager::sharedState()->pathForSFX(DESTROY_SFX)//"sfx/s886.ogg");
			, MAX_TO_DESTROY == -1 ? 
			1.0f : (1.0f + 0.5f * ((MAX_TO_DESTROY - remain) / (float)MAX_TO_DESTROY))
			, 1.f, 0.5
		);

		if (MAX_TO_DESTROY != -1) {
			remain--;
			remain = remain >= 0 ? remain : MAX_TO_DESTROY;
		}
		updateRemainObjectsLabel();
	}
};