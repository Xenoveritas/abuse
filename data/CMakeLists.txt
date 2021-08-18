INSTALL(FILES
    abuse.lsp
    edit.lsp
    hardness.lsp
DESTINATION ${ASSETDIR})

# No idea what this was for
# (note that while it could likely be included, it references a file that doesn't exist anyway)
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

# bong - example from crack.com
INSTALL(FILES
    addon/bong/bong.lsp
    addon/bong/bong01.lvl
    addon/bong/bong.spe
DESTINATION ${ASSETDIR}/addon/bong)

# deathmatch mode by crack.com
INSTALL(FILES
    addon/deathmat/cur_lev.lsp addon/deathmat/deathmat.lsp
    addon/deathmat/dstartup.lsp addon/deathmat/gamename.lsp
    addon/deathmat/large.lsp addon/deathmat/levelset.lsp
    addon/deathmat/medium.lsp addon/deathmat/small.lsp
    addon/deathmat/username.lsp addon/deathmat/version.lsp
DESTINATION ${ASSETDIR}/addon/deathmat)

# example - example addon by crack.com
INSTALL(FILES
    addon/example/example.lsp
    addon/example/example.lvl
    addon/example/example.spe
DESTINATION ${ASSETDIR}/addon/example)

# example game from crack.com
INSTALL(FILES
    addon/pong/common.lsp addon/pong/pong.lsp addon/pong/userfuns.lsp
    addon/pong/pong01.lvl
    addon/pong/pong.spe
    addon/pong/pong.txt
DESTINATION ${ASSETDIR}/addon/pong)

INSTALL(FILES
    addon/pong/levels/pongx01.spe addon/pong/levels/pongx02.spe
    addon/pong/levels/pongx03.spe addon/pong/levels/pongx04.spe
    addon/pong/levels/pongx05.spe addon/pong/levels/pongx06.spe
    addon/pong/levels/pongx07.spe addon/pong/levels/pongx08.spe
    addon/pong/levels/pongx09.spe addon/pong/levels/pongx10.spe
DESTINATION ${ASSETDIR}/addon/pong/levels)

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
    levels/demo1.dat
DESTINATION ${ASSETDIR}/levels)

INSTALL(FILES levels/mac/demo1.dat levels/mac/demo2.dat
    levels/mac/demo3.dat levels/mac/demo4.dat levels/mac/demo5.dat
DESTINATION ${ASSETDIR}/levels/mac)

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
else()
    message(WARNING "Non-free sound effects not found, sound will not be enabled.")
endif()
