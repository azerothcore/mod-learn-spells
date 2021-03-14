#include "Chat.h"
#include "Config.h"
#include "DisableMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

static bool learnlpells_announce;
static bool learnspells_enable;
static bool learnspells_onfirstlogin;
static bool learnspells_gmonly;
static uint32 learnspells_maxlevel;

class LearnAllSpellseBeforeConfigLoad : public WorldScript {
public:

    LearnAllSpellseBeforeConfigLoad() : WorldScript("LearnAllSpellseBeforeConfigLoad") { }

    void OnBeforeConfigLoad(bool /*reload*/) override {
        learnlpells_announce = sConfigMgr->GetBoolDefault("LearnSpells.Announce", 1);
        learnspells_enable = sConfigMgr->GetBoolDefault("LearnSpells.Enable", 1);
        learnspells_gmonly = sConfigMgr->GetBoolDefault("LearnSpells.OnlyGM", 1);
        learnspells_onfirstlogin = sConfigMgr->GetBoolDefault("LearnSpells.OnFirstLogin", 0);
        learnspells_maxlevel = sConfigMgr->GetIntDefault("learnspells_maxlevel", 80);
    }
};

class LearnSpellsOnLevelUp : public PlayerScript
{
  public:
    LearnSpellsOnLevelUp() : PlayerScript("LearnSpellsOnLevelUp") { }

    void OnLogin(Player* player) override
    {
        if (learnspells_enable && learnlpells_announce){
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00LearnAllSpells |rmodule.");
        }
    }

    void OnFirstLogin(Player* player) override
    {
        if (learnspells_onfirstlogin){
            LearnSpellsForNewLevel(player, 1);
        }
    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (learnspells_enable){
            if (!learnspells_gmonly || (learnspells_gmonly && player->GetSession()->GetSecurity() > 0))
            {
                if (player->getLevel() <= learnspells_maxlevel){
                    if (oldLevel < player->getLevel())
                        LearnSpellsForNewLevel(player, oldLevel);
                }
            }
        }
    }

  private:
    std::unordered_set<uint32> m_ignoreSpells = {
        64380, 23885, 23880, 44461, 25346, 10274, 10273, 8418,  8419,  7270,  7269,  7268,  54648, 12536, 24530, 70909, 12494, 57933, 24224, 27095, 27096, 27097, 27099, 32841, 56131, 56160, 56161, 48153, 34754, 64844, 64904, 48085, 33110, 48084,
        28276, 27874, 27873, 7001,  49821, 53022, 47757, 47750, 47758, 47666, 53001, 52983, 52998, 52986, 52987, 52999, 52984, 53002, 53003, 53000, 52988, 52985, 42208, 42209, 42210, 42211, 42212, 42213, 42198, 42937, 42938, 12484, 12485, 12486,
        44461, 55361, 55362, 34913, 43043, 43044, 38703, 38700, 27076, 42844, 42845, 64891, 25912, 25914, 25911, 25913, 25902, 25903, 27175, 27176, 33073, 33074, 48822, 48820, 48823, 48821, 20154, 25997, 20467, 20425, 67,    26017, 34471, 53254,
        13812, 14314, 14315, 27026, 49064, 49065, 60202, 60210, 13797, 14298, 14299, 14300, 14301, 27024, 49053, 49054, 52399, 1742,  24453, 53548, 53562, 52016, 26064, 35346, 57386, 57389, 57390, 57391, 57392, 57393, 55509, 35886, 43339, 45297,
        45298, 45299, 45300, 45301, 45302, 49268, 49269, 8349,  8502,  8503,  11306, 11307, 25535, 25537, 61650, 61654, 63685, 45284, 45286, 45287, 45288, 45289, 45290, 45291, 45292, 45293, 45294, 45295, 45296, 49239, 49240, 26364, 26365, 26366,
        26367, 26369, 26370, 26363, 26371, 26372, 49278, 49279, 32176, 32175, 21169, 47206, 27285, 47833, 47836, 42223, 42224, 42225, 42226, 42218, 47817, 47818, 42231, 42232, 42233, 42230, 48466, 44203, 44205, 44206, 44207, 44208, 48444, 48445,
        33891, 52374, 57532, 59921, 52372, 49142, 52375, 47633, 47632, 52373, 50536, 27214, 47822, 11682, 11681, 5857,  1010,  24907, 24905, 53227, 61391, 61390, 61388, 61387, 64801, 5421,  9635,  1178,  20186, 20185, 20184, 20187, 25899, 24406,
        50581, 30708, 48076, 62900, 62901, 62902, 59671, 50589, 66906, 66907, 24131, 23455, 23458, 23459, 27803, 27804, 27805, 25329, 48075, 42243, 42244, 42245, 42234, 58432, 58433, 65878, 18848, 16979, 49376, 54055, 20647, 42243, 24131,
        // COSMETIC SPELLS
        28271, 28272, 61025, 61305, 61721, 61780,
        // OPTIONAL QUEST SPELLS
        18540,
    };

    struct AddSpell // Additional Spell Entry
    {
        uint32 spellId;
        TeamId faction = TeamId::TEAM_NEUTRAL;
    };
    // { level: { SPELL_FAMILY_NAME: [{ spellId: uint32, faction: TeamId }] } }
    using SpellFamilyToExtraSpells = std::unordered_map<uint32, std::vector<AddSpell>>;
    using AdditionalSpellsList = std::unordered_map<uint8, SpellFamilyToExtraSpells>;
    // -------------------------------------------- ^^^^^ level

    AdditionalSpellsList m_additionalSpells = {
        {6,
         {
             {SPELLFAMILY_WARRIOR,
              {
                  AddSpell{3127}, // parry
              }},
         }},
        {8,
         {
             {SPELLFAMILY_HUNTER,
              {
                  AddSpell{3127}, // parry
              }},
             {SPELLFAMILY_PALADIN,
              {
                  AddSpell{3127}, // parry
              }},
         }},
        {10,
         {
             {SPELLFAMILY_HUNTER,
              {
                  AddSpell{1515}, // tame beast
              }},
         }},
        {12,
         {
             {SPELLFAMILY_ROGUE,
              {
                  AddSpell{3127}, // parry
              }},
         }},
        {14,
         {
             {SPELLFAMILY_HUNTER,
              {
                  AddSpell{6197}, // eagle eye
              }},
         }},
        {20,
         {
             {SPELLFAMILY_WARRIOR,
              {
                  AddSpell{674},   // dual wield
                  AddSpell{12678}, // stance mastery
              }},
             {SPELLFAMILY_HUNTER,
              {
                  AddSpell{674}, // dual wield
              }},
         }},
        {24,
         {
             {SPELLFAMILY_HUNTER,
              {
                  AddSpell{1462}, //  Beast Lore
                  AddSpell{19885}, // track hidden
              }},
             {SPELLFAMILY_ROGUE,
              {
                  AddSpell{2836}, //  Detect Traps
              }},
             {SPELLFAMILY_WARLOCK,
              {
                  AddSpell{5500}, //  Sense Demons
              }},
         }},
        {26,
         {
             {SPELLFAMILY_SHAMAN,
              {
                  AddSpell{6196}, //  Far Sight
              }},
         }},
        {30,
         {
             {SPELLFAMILY_SHAMAN,
              {
                  AddSpell{66842}, // Call of the Elements
              }},
         }},
        {32,
         {
             {SPELLFAMILY_DRUID,
              {
                  AddSpell{5225}, // Track Humanoids
              }},
         }},
        {40,
         {
             {SPELLFAMILY_SHAMAN,
              {
                  AddSpell{66843}, // Call of the Ancestors
                  AddSpell{8737}, // mail
              }},
             {SPELLFAMILY_HUNTER,
              {
                  AddSpell{8737}, // mail
              }},
             {SPELLFAMILY_DRUID,
              {
                  AddSpell{20719}, // Feline Grace
                  AddSpell{62600}, // Savage Defense
              }},
             {SPELLFAMILY_WARRIOR,
              {
                  AddSpell{750}, // plate mail
              }},
             {SPELLFAMILY_PALADIN,
              {
                  AddSpell{750}, // plate mail
              }},
         }},
        {50,
         {
             {SPELLFAMILY_SHAMAN,
              {
                  AddSpell{66844}, // Call of the Spirits
              }},
         }},
        {66,
         {
             {SPELLFAMILY_PALADIN,
              {
                  AddSpell{53736, TeamId::TEAM_HORDE},    // Seal of Corruption
                  AddSpell{31801, TeamId::TEAM_ALLIANCE}, // Seal of Vengeance
              }},
             {SPELLFAMILY_WARLOCK,
              {
                  AddSpell{29858}, // Soulshatter
              }},
         }},
        {70,
         {
             {SPELLFAMILY_SHAMAN,
              {
                  AddSpell{2825, TeamId::TEAM_HORDE},     // Bloodlust
                  AddSpell{32182, TeamId::TEAM_ALLIANCE}, // Heroism
              }},
         }},
        {80,
         {
             {SPELLFAMILY_WARLOCK,
              {
                  AddSpell{47836}, // Seed of Corruption (rank 3)
              }},
         }},
    };

    bool IsIgnoredSpell(uint32 spellID)
    {
        auto spellIt = m_ignoreSpells.find(spellID);
        return spellIt != m_ignoreSpells.end();
    }

    void LearnSpellsForNewLevel(Player* player, uint8 fromLevel)
    {
        uint8 upToLevel = player->getLevel();
        uint32 family = GetSpellFamily(player);

        for (int level = fromLevel; level <= upToLevel; level++)
        {
            ApplyAdditionalSpells(level, family, player);
            for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i);

                if (!spellInfo)
                    continue;
                if (spellInfo->SpellFamilyName != family)
                    continue;
                if ((spellInfo->AttributesEx7 & SPELL_ATTR7_ALLIANCE_ONLY && player->GetTeamId() != TEAM_ALLIANCE) || (spellInfo->AttributesEx7 & SPELL_ATTR7_HORDE_ONLY && player->GetTeamId() != TEAM_HORDE))
                    continue;
                if (spellInfo->PowerType == POWER_FOCUS)
                    continue;
                if (IsIgnoredSpell(spellInfo->Id))
                    continue;
                if (DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, spellInfo->Id, player))
                    continue;
                if (spellInfo->BaseLevel != uint32(level) && sSpellMgr->IsSpellValid(spellInfo))
                    continue;

                bool valid = false;

                SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellInfo->Id);

                for (SkillLineAbilityMap::const_iterator itr = bounds.first; itr != bounds.second; ++itr)
                {
                    if (itr->second->spellId == spellInfo->Id && itr->second->racemask == 0 && itr->second->learnOnGetSkill == 0)
                    {
                        valid = true;
                        SpellInfo const* prevSpell = spellInfo->GetPrevRankSpell();
                        if (prevSpell && !player->HasSpell(prevSpell->Id))
                        {
                            valid = false;
                            break;
                        }
                        if (GetTalentSpellPos(itr->second->spellId))
                            if (!prevSpell || !player->HasSpell(prevSpell->Id) || spellInfo->GetRank() == 1)
                                valid = false;
                        break;
                    }
                }

                if (valid)
                    player->learnSpell(spellInfo->Id);
            }
        }
    }

    void ApplyAdditionalSpells(uint8 level, uint32 playerSpellFamily, Player* player)
    {
        auto spells = m_additionalSpells.find(level);
        if (spells != m_additionalSpells.end())
        {
            SpellFamilyToExtraSpells spellsMap = spells->second;
            auto spellsForPlayersFamily = spellsMap.find(playerSpellFamily);
            if (spellsForPlayersFamily != spellsMap.end())
            {
                vector<AddSpell> additionalSpellsToTeach = spellsForPlayersFamily->second;
                for (auto const& spell : additionalSpellsToTeach)
                {
                    if (!(player->HasSpell(spell.spellId)) && (spell.faction == TeamId::TEAM_NEUTRAL || spell.faction == player->GetTeamId()))
                    {
                        player->learnSpell(spell.spellId);
                    }
                }
            }
        }
    }

    uint32 GetSpellFamily(const Player* p)
    {
        switch (p->getClass())
        {
        case CLASS_ROGUE:
            return SPELLFAMILY_ROGUE;
        case CLASS_DEATH_KNIGHT:
            return SPELLFAMILY_DEATHKNIGHT;
        case CLASS_WARRIOR:
            return SPELLFAMILY_WARRIOR;
        case CLASS_PRIEST:
            return SPELLFAMILY_PRIEST;
        case CLASS_MAGE:
            return SPELLFAMILY_MAGE;
        case CLASS_PALADIN:
            return SPELLFAMILY_PALADIN;
        case CLASS_HUNTER:
            return SPELLFAMILY_HUNTER;
        case CLASS_DRUID:
            return SPELLFAMILY_DRUID;
        case CLASS_SHAMAN:
            return SPELLFAMILY_SHAMAN;
        case CLASS_WARLOCK:
            return SPELLFAMILY_WARLOCK;
        default:
            return SPELLFAMILY_GENERIC;
        }
    }
};

void AddLearnAllSpellsScripts()
{
    new LearnAllSpellseBeforeConfigLoad();
    new LearnSpellsOnLevelUp();
}
