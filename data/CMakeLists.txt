INSTALL(FILES
    abuse.lsp
    edit.lsp
    hardness.lsp
DESTINATION ${ASSETDIR})

# No idea what this was for
#INSTALL(FILES music/playhmp.lsp DESTINATION ${ASSETDIR}/music)

INSTALL(FILES
    lisp/ant.lsp
    lisp/chat.lsp
    lisp/common.lsp
    lisp/doors.lsp
    lisp/duong.lsp
    lisp/english.lsp
    lisp/explo.lsp
    lisp/flyer.lsp
    lisp/french.lsp
    lisp/gates.lsp
    lisp/general.lsp
    lisp/german.lsp
    lisp/guns.lsp
    lisp/input.lsp
    lisp/jugger.lsp
    lisp/ladder.lsp
    lisp/language.lsp
    lisp/light.lsp
    lisp/options.lsp
    lisp/people.lsp
    lisp/platform.lsp
    lisp/playwav.lsp
    lisp/powerup.lsp
    lisp/sfx.lsp
    lisp/share.lsp
    lisp/startup.lsp
    lisp/switch.lsp
    lisp/teleport.lsp
    lisp/userfuns.lsp
    lisp/version.lsp
    lisp/weapons.lsp
DESTINATION ${ASSETDIR}/lisp)

INSTALL(FILES
    addon/aliens/alichars.lsp addon/aliens/aliens.lsp
    addon/aliens/astartup.lsp addon/aliens/objects.lsp
    addon/aliens/powerold.lsp addon/aliens/powerups.lsp
    addon/aliens/sfx.lsp addon/aliens/tiles.lsp
DESTINATION ${ASSETDIR}/addon/aliens)

INSTALL(FILES
    addon/bong/bong.lsp
DESTINATION ${ASSETDIR}/addon/bong)

INSTALL(FILES
    addon/claudio/claudio.lsp addon/claudio/oldclaud.lsp
DESTINATION ${ASSETDIR}/addon/claudio)

INSTALL(FILES
    addon/deathmat/cur_lev.lsp addon/deathmat/deathmat.lsp
    addon/deathmat/dstartup.lsp addon/deathmat/gamename.lsp
    addon/deathmat/large.lsp addon/deathmat/levelset.lsp
    addon/deathmat/medium.lsp addon/deathmat/small.lsp
    addon/deathmat/username.lsp addon/deathmat/version.lsp
DESTINATION ${ASSETDIR}/addon/deathmat)

INSTALL(FILES
    addon/example/example.lsp
DESTINATION ${ASSETDIR}/addon/example)

INSTALL(FILES
    addon/leon/4frabsdm.lsp addon/leon/4frabs.lsp addon/leon/leon.lsp
DESTINATION ${ASSETDIR}/addon/leon)

INSTALL(FILES
    addon/leon/lisp/deco.lsp addon/leon/lisp/grenade.lsp
    addon/leon/lisp/lmisc.lsp addon/leon/lisp/lnant.lsp
    addon/leon/lisp/nextlev3.lsp addon/leon/lisp/nguns.lsp
    addon/leon/lisp/njug.lsp addon/leon/lisp/rain.lsp
    addon/leon/lisp/slavesw.lsp addon/leon/lisp/text.lsp
DESTINATION ${ASSETDIR}/addon/leon/lisp)

INSTALL(FILES
    addon/newart/newart.lsp
DESTINATION ${ASSETDIR}/addon/newart)

INSTALL(FILES
    addon/pong/common.lsp addon/pong/pong.lsp addon/pong/userfuns.lsp
DESTINATION ${ASSETDIR}/addon/pong)

INSTALL(FILES
    addon/twist/f2ai.lsp addon/twist/f2chars.lsp
    addon/twist/twist.lsp
DESTINATION ${ASSETDIR}/addon/twist)

INSTALL(FILES
    addon/twist/lisp/ai.lsp
    addon/twist/lisp/chars.lsp addon/twist/lisp/chat.lsp
    addon/twist/lisp/dray.lsp addon/twist/lisp/english.lsp
    addon/twist/lisp/f2ai.lsp addon/twist/lisp/f2chars.lsp
    addon/twist/lisp/light.lsp addon/twist/lisp/mario.lsp
    addon/twist/lisp/objects.lsp addon/twist/lisp/options.lsp
    addon/twist/lisp/players.lsp addon/twist/lisp/sfx.lsp
    addon/twist/lisp/startup.lsp addon/twist/lisp/tiles.lsp
    addon/twist/lisp/tints.lsp addon/twist/lisp/title.lsp
    addon/twist/lisp/userfuns.lsp addon/twist/lisp/weapons.lsp
DESTINATION ${ASSETDIR}/addon/twist/lisp)

INSTALL(FILES
    art/ant.spe art/ball.spe art/blowups.spe art/bold.spe art/boss.spe
    art/cloud.spe art/compass.spe art/consfnt.spe art/cop.spe art/coptop.spe
    art/credit.spe art/dev.spe art/door.spe art/exp1.spe art/flyer.spe
    art/fonts.spe art/frame.spe art/gun2.spe art/help.spe art/icons.spe
    art/jug.spe art/keys.spe art/letters.spe art/loading.spe art/misc.spe
    art/missle.spe art/mon_cfg.spe art/pform.spe art/rob1.spe art/rob2.spe
    art/screen11.spe art/smoke.spe art/statbar.spe art/status.spe
    art/title.spe art/wait.spe
DESTINATION ${ASSETDIR}/art)

INSTALL(FILES
    art/back/alienb.spe art/back/backgrnd.spe art/back/cave.spe
    art/back/city.spe art/back/galien.spe art/back/green2.spe
    art/back/intro.spe art/back/tech.spe
DESTINATION ${ASSETDIR}/art/back)

INSTALL(FILES
    art/chars/ammo.spe art/chars/block.spe art/chars/concus.spe
    art/chars/door.spe art/chars/lavap.spe art/chars/lava.spe
    art/chars/lightin.spe art/chars/mine.spe art/chars/platform.spe
    art/chars/push.spe art/chars/sect.spe art/chars/step.spe
    art/chars/tdoor.spe art/chars/teleport.spe
DESTINATION ${ASSETDIR}/art/chars)

INSTALL(FILES
    art/fore/alien.spe art/fore/cave.spe art/fore/endgame.spe
    art/fore/foregrnd.spe art/fore/techno2.spe art/fore/techno3.spe
    art/fore/techno4.spe art/fore/techno.spe art/fore/trees2.spe
    art/fore/trees.spe
DESTINATION ${ASSETDIR}/art/fore)

INSTALL(FILES
    art/tints/ant/blue.spe art/tints/ant/brown.spe
    art/tints/ant/darkblue.spe art/tints/ant/egg.spe
    art/tints/ant/evil.spe art/tints/ant/gray.spe
    art/tints/ant/green.spe art/tints/ant/mustard.spe
    art/tints/ant/orange.spe art/tints/ant/yellow.spe
DESTINATION ${ASSETDIR}/art/tints/ant)

INSTALL(FILES
    art/tints/cop/africa.spe art/tints/cop/blue.spe
    art/tints/cop/bright.spe art/tints/cop/darkblue.spe
    art/tints/cop/fire.spe art/tints/cop/gold.spe
    art/tints/cop/gray.spe art/tints/cop/land.spe
    art/tints/cop/olive.spe art/tints/cop/pinkish.spe
    art/tints/cop/purple.spe art/tints/cop/yellow.spe
DESTINATION ${ASSETDIR}/art/tints/cop)

INSTALL(FILES
    art/tints/guns/blue.spe art/tints/guns/green.spe
    art/tints/guns/orange.spe art/tints/guns/redish.spe
DESTINATION ${ASSETDIR}/art/tints/guns)

INSTALL(FILES
    addon/aliens/aliens.spe addon/aliens/bactiles.spe
    addon/aliens/fortiles.spe
DESTINATION ${ASSETDIR}/addon/aliens)

INSTALL(FILES
    addon/bong/bong.spe
DESTINATION ${ASSETDIR}/addon/bong)

INSTALL(FILES
    addon/claudio/antship.spe addon/claudio/bigexp.spe
    addon/claudio/deepw1.spe addon/claudio/deepw2.spe addon/claudio/droid.spe
    addon/claudio/extiles.spe addon/claudio/fire.spe addon/claudio/lamp.spe
    addon/claudio/lava2.spe addon/claudio/mypanels.spe
    addon/claudio/nplatfor.spe addon/claudio/pal21.spe addon/claudio/pal2.spe
    addon/claudio/pal5.spe addon/claudio/pal81f.spe addon/claudio/pal81.spe
    addon/claudio/pal82f.spe addon/claudio/pal82.spe addon/claudio/pal90.spe
    addon/claudio/rob2.spe addon/claudio/skull.spe addon/claudio/spaced.spe
    addon/claudio/spaceh.spe addon/claudio/spacymed.spe
    addon/claudio/spacytdm.spe addon/claudio/sswitch.spe
    addon/claudio/t_lamp.spe addon/claudio/trex1.spe
    addon/claudio/t_skull.spe addon/claudio/t_space.spe
    addon/claudio/t_trex.spe addon/claudio/t_water.spe
    addon/claudio/umbrel3.spe addon/claudio/watem.spe
DESTINATION ${ASSETDIR}/addon/claudio)

INSTALL(FILES
    addon/example/example.spe
DESTINATION ${ASSETDIR}/addon/example)

INSTALL(FILES
    addon/leon/gray.spe addon/leon/level00.spe addon/leon/level01.spe
    addon/leon/level02.spe addon/leon/lmisc.spe addon/leon/lnant.spe
    addon/leon/lnewft2.spe addon/leon/lnewft.spe addon/leon/rain-old.spe
    addon/leon/rain.spe
DESTINATION ${ASSETDIR}/addon/leon)

INSTALL(FILES
    addon/newart/blcave.spe addon/newart/final.spe addon/newart/frabs_1.spe
    addon/newart/frabs_2.spe addon/newart/frbsblue.spe addon/newart/mtile.spe
    addon/newart/space.spe addon/newart/stones.spe
DESTINATION ${ASSETDIR}/addon/newart)

INSTALL(FILES
    addon/pong/levels/pongx01.spe addon/pong/levels/pongx02.spe
    addon/pong/levels/pongx03.spe addon/pong/levels/pongx04.spe
    addon/pong/levels/pongx05.spe addon/pong/levels/pongx06.spe
    addon/pong/levels/pongx07.spe addon/pong/levels/pongx08.spe
    addon/pong/levels/pongx09.spe addon/pong/levels/pongx10.spe
DESTINATION ${ASSETDIR}/addon/pong/levels)

INSTALL(FILES
    addon/pong/pong.spe
DESTINATION ${ASSETDIR}/addon/pong)

INSTALL(FILES
    addon/twist/art/cgc.spe addon/twist/art/dray.spe addon/twist/art/fire.spe
    addon/twist/art/lavap.spe addon/twist/art/legs.spe
    addon/twist/art/mario.spe addon/twist/art/mtile.spe
    addon/twist/art/obj.spe addon/twist/art/palette.spe
    addon/twist/art/robs.spe addon/twist/art/title.spe
    addon/twist/art/weapons.spe
DESTINATION ${ASSETDIR}/addon/twist/art)

INSTALL(FILES
    levels/level00.spe levels/level01.spe levels/level02.spe
    levels/level03.spe levels/level04.spe levels/level05.spe
    levels/level06.spe levels/level07.spe levels/level08.spe
    levels/level09.spe levels/level10.spe levels/level11.spe
    levels/level12.spe levels/level13.spe levels/level14.spe
    levels/level15.spe levels/level16.spe levels/level17.spe
    levels/level18.spe levels/level19.spe levels/level20.spe
    levels/level21.spe
DESTINATION ${ASSETDIR}/levels)

INSTALL(FILES
    levels/frabs00.spe levels/frabs01.spe levels/frabs02.spe
    levels/frabs03.spe levels/frabs04.spe levels/frabs05.spe
    levels/frabs06.spe levels/frabs07.spe levels/frabs08.spe
    levels/frabs09.spe levels/frabs10.spe levels/frabs11.spe
    levels/frabs12.spe levels/frabs13.spe levels/frabs14.spe
    levels/frabs15.spe levels/frabs17.spe levels/frabs18.spe
    levels/frabs19.spe levels/frabs20.spe levels/frabs21.spe
    levels/frabs30.spe levels/frabs70.spe levels/frabs71.spe
    levels/frabs72.spe levels/frabs73.spe levels/frabs74.spe
DESTINATION ${ASSETDIR}/levels)

INSTALL(FILES
    levels/mac/demo1.spe levels/mac/demo2.spe levels/mac/demo4.spe
    levels/mac/demo5.spe levels/mac/end.spe levels/mac/flevel12.spe
    levels/mac/level00.spe levels/mac/level01.spe levels/mac/level02.spe
    levels/mac/level05.spe levels/mac/level07.spe levels/mac/level08.spe
    levels/mac/level09.spe levels/mac/level12.spe levels/mac/level13.spe
    levels/mac/level15.spe levels/mac/level18.spe levels/mac/level19.spe
    levels/mac/level20.spe levels/mac/levels.spe levels/mac/netreg1.spe
    levels/mac/netshar1.spe levels/mac/netshar2.spe
DESTINATION ${ASSETDIR}/levels/mac)

INSTALL(FILES
    netlevel/00dm.spe netlevel/2play1.spe netlevel/2play2.spe
    netlevel/2play3.spe netlevel/2play4.spe netlevel/4play1.spe
    netlevel/4play2.spe netlevel/4play3.spe netlevel/4play4.spe
    netlevel/8play1.spe netlevel/8play2.spe netlevel/8play3.spe
    netlevel/8play4.spe netlevel/alitlhot.spe netlevel/aquarius.spe
    netlevel/bugsmed.spe netlevel/bugssml.spe netlevel/bugs.spe
    netlevel/cistern.spe netlevel/getcross.spe netlevel/jdm3.spe
    netlevel/kotcross.spe netlevel/laazrckt.spe netlevel/limeston.spe
    netlevel/madrace.spe netlevel/occult.spe netlevel/redgrndm.spe
    netlevel/ruins.spe netlevel/spacymed.spe netlevel/spacytdm.spe
    netlevel/teardrop.spe netlevel/treesmal.spe
DESTINATION ${ASSETDIR}/netlevel)

INSTALL(FILES
    levels/mac/pong.lvl
DESTINATION ${ASSETDIR}/levels/mac)

INSTALL(FILES
    addon/bong/bong01.lvl
DESTINATION ${ASSETDIR}/addon/bong)

INSTALL(FILES
    addon/example/example.lvl
DESTINATION ${ASSETDIR}/addon/example)

INSTALL(FILES
    addon/pong/pong01.lvl
DESTINATION ${ASSETDIR}/addon/pong)

INSTALL(FILES
    addon/twist/levels/l01s01.lvl addon/twist/levels/l01s02.lvl
    addon/twist/levels/l01s03.lvl addon/twist/levels/l01s04.lvl
DESTINATION ${ASSETDIR}/addon/twist/levels)

INSTALL(FILES
    levels/demo1.dat
DESTINATION ${ASSETDIR}/levels)

INSTALL(FILES levels/mac/demo1.dat levels/mac/demo2.dat
    levels/mac/demo3.dat levels/mac/demo4.dat levels/mac/demo5.dat
DESTINATION ${ASSETDIR}/levels/mac)

INSTALL(FILES
    addon/aliens/aliens.txt
    addon/aliens/readme.txt
DESTINATION ${ASSETDIR}/addon/aliens)

INSTALL(FILES
    addon/claudio/palettes.txt
DESTINATION ${ASSETDIR}/addon/claudio)

INSTALL(FILES
    addon/leon/leon.txt
DESTINATION ${ASSETDIR}/addon/leon)

INSTALL(FILES
    addon/newart/tiledoc.txt
DESTINATION ${ASSETDIR}/addon/newart)

INSTALL(FILES
    addon/pong/pong.txt
DESTINATION ${ASSETDIR}/addon/pong)

INSTALL(FILES
    addon/twist/readme.txt
DESTINATION ${ASSETDIR}/addon/twist)

INSTALL(FILES
    defaults.prp
DESTINATION ${ASSETDIR})

# Check if we have the non-free data
set(IGNORE_NONFREE off CACHE BOOL "ignore non-free music/SFX even if it exists")
if(IGNORE_NONFREE)
    message(WARNING "Skipping non-free music (set to ignore)")
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/music)
    message(WARNING "Non-free music found. Build will include non-free data!")
    INSTALL(FILES
        music/intro.hmi music/victory.hmi
        music/abuse00.hmi music/abuse01.hmi music/abuse02.hmi music/abuse03.hmi
        music/abuse04.hmi music/abuse06.hmi music/abuse08.hmi music/abuse09.hmi
        music/abuse11.hmi music/abuse13.hmi music/abuse15.hmi music/abuse17.hmi
    DESTINATION ${ASSETDIR}/music)
else()
    message(WARNING "Non-free music not found, it will not be included.")
endif()

if(IGNORE_NONFREE)
    message(WARNING "Skipping non-free SFX (set to ignore)")
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sfx)
    message(WARNING "Non-free sound effects found. Build will include non-free data!")
    INSTALL(FILES
        sfx/adie02.wav sfx/adie03.wav sfx/adie05.wav sfx/ahit01.wav
        sfx/aland01.wav sfx/alien01.wav sfx/amb07.wav sfx/amb10.wav
        sfx/amb11.wav sfx/amb13.wav sfx/amb15.wav sfx/amb16.wav
        sfx/ambcave1.wav sfx/ambcave2.wav sfx/ambcave3.wav sfx/ambcave4.wav
        sfx/ambfrst2.wav sfx/ambtech1.wav sfx/ambtech2.wav sfx/ambtech3.wav
        sfx/ammo01.wav sfx/ammo02.wav sfx/apain01.wav sfx/aslash01.wav
        sfx/ball01.wav sfx/blkfoot4.wav sfx/button02.wav sfx/cleaner.wav
        sfx/crmble01.wav sfx/delobj01.wav sfx/doorup01.wav sfx/doorup02.wav
        sfx/eleacc01.wav sfx/elect02.wav sfx/eledec01.wav sfx/endlvl02.wav
        sfx/explod02.wav sfx/fadeon01.wav sfx/firebmb1.wav sfx/flamloop.wav
        sfx/fly03.wav sfx/force01.wav sfx/grenad01.wav sfx/health01.wav
        sfx/lasrmis2.wav sfx/lava01.wav sfx/link01.wav sfx/logo09.wav
        sfx/metal.wav sfx/mghit01.wav sfx/mghit02.wav sfx/pland01.wav
        sfx/plasma02.wav sfx/plasma03.wav sfx/pldeth02.wav sfx/pldeth04.wav
        sfx/pldeth05.wav sfx/pldeth07.wav sfx/plpain01.wav sfx/plpain02.wav
        sfx/plpain04.wav sfx/plpain10.wav sfx/poof05.wav sfx/poof06.wav
        sfx/robot02.wav sfx/rocket02.wav sfx/save01.wav sfx/save05.wav
        sfx/scream02.wav sfx/scream03.wav sfx/scream08.wav sfx/shotgn31.wav
        sfx/speed02.wav sfx/spring03.wav sfx/swish01.wav sfx/switch01.wav
        sfx/telept01.wav sfx/throw01.wav sfx/timerfst.wav sfx/zap2.wav
        sfx/zap3.wav
    DESTINATION ${ASSETDIR}/sfx)

    INSTALL(FILES
        sfx/voice/aimsave.wav sfx/voice/ammosave.wav sfx/voice/ladder_1.wav
        sfx/voice/ownsave.wav sfx/voice/platfo_1.wav sfx/voice/poweru_1.wav
        sfx/voice/savesave.wav sfx/voice/spaceb_1.wav sfx/voice/spcbr1.wav
        sfx/voice/spcbr2.wav sfx/voice/starts_1.wav sfx/voice/statio_1.wav
        sfx/voice/switch_1.wav sfx/voice/switch_2.wav sfx/voice/telepo_1.wav
        sfx/voice/wallss_1.wav sfx/voice/weapon_1.wav
    DESTINATION ${ASSETDIR}/sfx/voice)

    # The rest are in the free-only repository but by missing the SFX,
    # the sound engine will be deactivated anyway. C'est la vie.

    INSTALL(FILES
        addon/aliens/adie01.wav addon/aliens/adie02.wav addon/aliens/adie03.wav
        addon/aliens/adie05.wav addon/aliens/ahit01.wav addon/aliens/alien01.wav
        addon/aliens/altaunt.wav addon/aliens/aslash01.wav
        addon/aliens/egghatch.wav addon/aliens/jarbreak.wav
    DESTINATION ${ASSETDIR}/addon/aliens)

    INSTALL(FILES
        addon/claudio/aship.wav addon/claudio/drill.wav addon/claudio/fire.wav
        addon/claudio/glass1.wav addon/claudio/sewers.wav addon/claudio/skull.wav
        addon/claudio/spaceo.wav addon/claudio/trex1.wav addon/claudio/trex2.wav
        addon/claudio/unhealth.wav addon/claudio/wfall.wav
    DESTINATION ${ASSETDIR}/addon/claudio)

    INSTALL(FILES
        addon/leon/sfx/ambship1.wav addon/leon/sfx/ambship2.wav
        addon/leon/sfx/gren5.wav addon/leon/sfx/gren6.wav
        addon/leon/sfx/rain2.wav addon/leon/sfx/rain3.wav
        addon/leon/sfx/rain4.wav addon/leon/sfx/rain.wav
        addon/leon/sfx/thunder2.wav addon/leon/sfx/thunder3.wav
        addon/leon/sfx/thunder4.wav addon/leon/sfx/thunder5.wav
        addon/leon/sfx/thunder.wav
    DESTINATION ${ASSETDIR}/addon/leon/sfx)

    INSTALL(FILES
        addon/twist/sfx/dray.wav
    DESTINATION ${ASSETDIR}/addon/twist/sfx)
else()
    message(WARNING "Non-free sound effects not found, sound will not be enabled.")
endif()
