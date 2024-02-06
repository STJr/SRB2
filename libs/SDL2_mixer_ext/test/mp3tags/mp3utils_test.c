
#include "SDL_test.h"

#define ENABLE_ALL_MP3_TAGS
#include "mp3utils.h"

static void verify_file(const char *path, SDL_bool keep_id3v2,
                       Sint64 begin, Sint64 length,
                       const char *title,
                       const char *artist,
                       const char *album,
                       const char *copyright, int wanted_ret)
{
    Mix_MusicMetaTags tags;
    struct mp3file_t f;
    int ret = 0;
    const char *t_title, *t_artist, *t_album, *t_copyright;

    meta_tags_init(&tags);
    SDL_memset(&f, 0, sizeof(struct mp3file_t));

    f.src = SDL_RWFromFile(path, "rb");

    SDLTest_AssertCheck(f.src != NULL, "Check that file opening is valid");

    if (!f.src) {
        meta_tags_clear(&tags);
        return;
    }

    f.length = SDL_RWsize(f.src);

    ret = mp3_read_tags(&tags, &f, keep_id3v2);
    SDLTest_AssertCheck(ret == wanted_ret, "Check that tag reading is valid or invalid (%d got, %d wanted)", ret, wanted_ret);

    SDLTest_AssertCheck(f.start == begin,   "Check that start position is matches %d==%d value", (int)f.start, (int)begin);
    SDLTest_AssertCheck(f.length == length, "Check that audio chunk length is matches to %d==%d value", (int)f.length, (int)length);

    t_title = meta_tags_get(&tags, MIX_META_TITLE);
    t_artist = meta_tags_get(&tags, MIX_META_ARTIST);
    t_album = meta_tags_get(&tags, MIX_META_ALBUM);
    t_copyright = meta_tags_get(&tags, MIX_META_COPYRIGHT);

    SDLTest_AssertCheck(SDL_strncmp(t_title, title, SDL_strlen(title)) == 0,
                        "Check that title tag is matches \"%s\" == \"%s\" value", t_title, title);
    SDLTest_AssertCheck(SDL_strncmp(t_artist, artist, SDL_strlen(artist)) == 0,
                        "Check that artist tag is matches \"%s\" == \"%s\" value", t_artist, artist);
    SDLTest_AssertCheck(SDL_strncmp(t_album, album, SDL_strlen(album)) == 0,
                        "Check that album tag is matches \"%s\" == \"%s\" value", t_album, album);
    SDLTest_AssertCheck(SDL_strncmp(t_copyright, copyright, SDL_strlen(copyright)) == 0,
                        "Check that copyright tag is matches \"%s\" == \"%s\" value", t_copyright, copyright);

    meta_tags_clear(&tags);
}

static int tag_notags(void *arg)
{
    (void)arg;
    verify_file("notags.mp3", SDL_FALSE, 0, 2869, "", "", "", "", 0);
    return TEST_COMPLETED;
}

static int tag_ap2v1_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("APEv1+ID3v1tag.mp3", SDL_FALSE,
                0, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks", 0);

    return TEST_COMPLETED;
}


static int tag_ap2v1(void *arg)
{
    (void)arg;
    verify_file("APEv1tag.mp3", SDL_FALSE,
                0, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks", 0);

    return TEST_COMPLETED;
}

static int tag_ap2v2_at_begin_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("APEv2AtBegin+ID3v1tag.mp3", SDL_FALSE,
                162, 2869,
                "Abrakudbra",
                "Some odd jerk",
                "Odd things of Kek",
                "", 0);
    return TEST_COMPLETED;
}

static int tag_ap2v2_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("APEv2+ID3v1tag.mp3", SDL_FALSE,
                0, 2869,
                "Abrakudbra",
                "Some odd jerk",
                "Odd things of Kek",
                "", 0);
    return TEST_COMPLETED;
}

static int tag_ap2v2(void *arg)
{
    (void)arg;
    verify_file("APEv2tag.mp3", SDL_FALSE,
                0, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks", 0);
    return TEST_COMPLETED;
}

static int tag_ap2v2_at_begin(void *arg)
{
    (void)arg;
    verify_file("APEv2tagAtBegin.mp3", SDL_FALSE,
                239, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks", 0);
    return TEST_COMPLETED;
}

static int tag_ap2v2_and_lyrics3v2(void *arg)
{
    (void)arg;
    verify_file("APEv2tagMixedWithLyrics3.mp3", SDL_FALSE,
                0, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks", 0);
    return TEST_COMPLETED;
}

static int tag_ap2v2_and_tow_lyrics3v2(void *arg)
{
    (void)arg;
    verify_file("APEv2tagMixedWithLyrics3EvenWorse.mp3", SDL_FALSE,
                0, 3029, /* 3029 is when a second Lyrics3v2 is not handled, and it's should be 2869 when handling all tags */
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks", 0);
    return TEST_COMPLETED;
}


static int tag_id3v1(void *arg)
{
    (void)arg;
    verify_file("id3v1tag.mp3", SDL_FALSE,
                0, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", 0);
    return TEST_COMPLETED;
}

static int tag_id3v22_with_image(void *arg)
{
    (void)arg;
    verify_file("id3v22obsolete-2.mp3", SDL_FALSE,
                247202, 2869, "", "", "", "", 0);
    return TEST_COMPLETED;
}

static int tag_id3v22_noimage(void *arg)
{
    (void)arg;
    verify_file("id3v22obsolete-noimage-2.mp3", SDL_FALSE,
                10446, 2869,
                "NAMEABCDEFGHIJKLMNOPQRSTUVWXYZ",
                "ARTISTABCDEFGHIJKLMNOPQRSTUVWXYZ",
                "ALBUMNAMEABCDEFGHIJKLMNOPQRSTUVWXYZ",
                "", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23(void *arg)
{
    (void)arg;
    verify_file("id3v23tag.mp3", SDL_FALSE,
                1099, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23_with_bmp(void *arg)
{
    (void)arg;
    verify_file("id3v23tagwithbpm.mp3", SDL_FALSE,
                501, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23_with_bmp_float(void *arg)
{
    (void)arg;
    verify_file("id3v23tagwithbpmfloat.mp3", SDL_FALSE,
                504, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23_with_bmp_float_comma(void *arg)
{
    (void)arg;
    verify_file("id3v23tagwithbpmfloatwithcomma.mp3", SDL_FALSE,
                504, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23_with_chapters(void *arg)
{
    (void)arg;
    verify_file("id3v23tagwithchapters.mp3", SDL_FALSE,
                1412, 90504, "ID3 chapters example", "", "", "", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23_with_wmp_rating(void *arg)
{
    (void)arg;
    verify_file("id3v23tagwithwmprating.mp3", SDL_FALSE,
                529, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}

static int tag_id3v23_unicode_tags(void *arg)
{
    (void)arg;
    verify_file("id3v23unicodetags.mp3", SDL_FALSE,
                202, 2869,
                "中文",
                "γειά σου",
                "こんにちは",
                "", 0);
    return TEST_COMPLETED;
}

static int tag_id3v24(void *arg)
{
    (void)arg;
    verify_file("id3v24tagswithalbumimage.mp3", SDL_FALSE,
                3147, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}


static int tag_lyrics3(void *arg)
{
    (void)arg;
    verify_file("lyrics3v1.mp3", SDL_FALSE,
                0, 2869, "", "", "", "", 0);
    return TEST_COMPLETED;
}

static int tag_lyrics3_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("lyrics3v1withID3v1.mp3", SDL_FALSE,
                0, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", 0);
    return TEST_COMPLETED;
}

static int tag_lyrics3_with_id3v1_invalid_file(void *arg)
{
    (void)arg;
    verify_file("lyrics3v1withID3v1-invalid.mp3", SDL_FALSE,
                0, 3026,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", -1);
    return TEST_COMPLETED;
}

static int tag_lyrics3_with_id3v1_invalid_tail(void *arg)
{
    (void)arg;
    verify_file("lyrics3v1withID3v1-invalid-tail.mp3", SDL_FALSE,
                0, 3026,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", 0);
    return TEST_COMPLETED;
}

static int tag_lyrics3v2(void *arg)
{
    (void)arg;
    verify_file("lyrics3v2.mp3", SDL_FALSE,
                0, 2869, "", "", "", "", 0);
    return TEST_COMPLETED;
}

static int tag_lyrics3v2_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("lyrics3v2withID3v1.mp3", SDL_FALSE,
                0, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", 0);
    return TEST_COMPLETED;
}

static int tag_lyrics3v2_with_id3v1_invalid_file(void *arg)
{
    (void)arg;
    verify_file("lyrics3v2withID3v1-invalid.mp3", SDL_FALSE,
                0, 2947,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", -1);
    return TEST_COMPLETED;
}

static int tag_lyrics3v2_with_id3v1_invalid_tail(void *arg)
{
    (void)arg;
    verify_file("lyrics3v2withID3v1-invalid-tail.mp3", SDL_FALSE,
                0, 2947,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", 0);
    return TEST_COMPLETED;
}



static int tag_musicmatch(void *arg)
{
    (void)arg;
    verify_file("musicmatch.mp3", SDL_FALSE,
                0, 2869, "", "", "", "", 0);
    return TEST_COMPLETED;
}

static int tag_musicmatch_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("musicmatchWithID3v1.mp3", SDL_FALSE,
                0, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COMMENT123456789012345678901", 0);
    return TEST_COMPLETED;
}

static int tag_musicmatch_with_id3v1_and_2(void *arg)
{
    (void)arg;
    verify_file("musicmatchWithID3v1andID3v2.mp3", SDL_FALSE,
                1099, 2869,
                "TITLE1234567890123456789012345",
                "ARTIST123456789012345678901234",
                "ALBUM1234567890123456789012345",
                "COPYRIGHT2345678901234567890123", 0);
    return TEST_COMPLETED;
}

static const SDLTest_TestCaseReference noTagsTest =
        { (SDLTest_TestCaseFp)tag_notags,"tag_notags",  "Tests of a file with no tags", TEST_ENABLED };

static const SDLTest_TestCaseReference apeTest1 =
        { (SDLTest_TestCaseFp)tag_ap2v1_with_id3v1,"tag_ap2v1_with_id3v1",  "Tests APEv1 tag with leading ID3v1", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest2 =
        { (SDLTest_TestCaseFp)tag_ap2v1,"tag_ap2v1",  "Tests APEv1 tag", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest3 =
        { (SDLTest_TestCaseFp)tag_ap2v2_at_begin_with_id3v1,"tag_ap2v2_at_begin_with_id3v1",  "Tests APEv2 tag at file begin and leading ID3v1", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest4 =
        { (SDLTest_TestCaseFp)tag_ap2v2_with_id3v1,"tag_ap2v2_with_id3v1",  "Tests APEv2 tag with leading ID3v1", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest5 =
        { (SDLTest_TestCaseFp)tag_ap2v2,"tag_ap2v2",  "Tests APEv2 tag", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest6 =
        { (SDLTest_TestCaseFp)tag_ap2v2_at_begin,"tag_ap2v2_at_begin",  "Tests APEv2 tag at file begin", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest7 =
        { (SDLTest_TestCaseFp)tag_ap2v2_and_lyrics3v2,"tag_ap2v2_and_lyrics3v2",  "Tests APEv2 tag at file begin and Lyrics3", TEST_ENABLED };
static const SDLTest_TestCaseReference apeTest8 =
        { (SDLTest_TestCaseFp)tag_ap2v2_and_tow_lyrics3v2,"tag_ap2v2_and_tow_lyrics3v2",  "Tests APEv2 tag at file begin and Lyrics3 (has two Lyrics3v2 tags)", TEST_ENABLED };

static const SDLTest_TestCaseReference id3v1Test1 =
        { (SDLTest_TestCaseFp)tag_id3v1,"tag_id3v1",  "Tests ID3v1 tag", TEST_ENABLED };

static const SDLTest_TestCaseReference id3v22Test1 =
        { (SDLTest_TestCaseFp)tag_id3v22_with_image,"tag_id3v2",  "Tests ID3v2.2 tag with image", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v22Test2 =
        { (SDLTest_TestCaseFp)tag_id3v22_noimage,"tag_id3v2_noimage",  "Tests ID3v2.2 tag with no image", TEST_ENABLED };


static const SDLTest_TestCaseReference id3v23Test1 =
        { (SDLTest_TestCaseFp)tag_id3v23,"tag_id3v3",  "Tests ID3v2.3 tag", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v23Test2 =
        { (SDLTest_TestCaseFp)tag_id3v23_with_bmp,"tag_id3v23_with_bmp",  "Tests ID3v2.3 tag (with BPM)", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v23Test3 =
        { (SDLTest_TestCaseFp)tag_id3v23_with_bmp_float,"tag_id3v23_with_bmp_float",  "Tests ID3v2.3 tag (with BPM float)", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v23Test4 =
        { (SDLTest_TestCaseFp)tag_id3v23_with_bmp_float_comma,"tag_id3v23_with_bmp_float_comma",  "Tests ID3v2.3 tag (with BPM float with comma)", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v23Test5 =
        { (SDLTest_TestCaseFp)tag_id3v23_with_chapters,"tag_id3v23_with_characters",  "Tests ID3v2.3 tag (with chapters)", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v23Test6 =
        { (SDLTest_TestCaseFp)tag_id3v23_with_wmp_rating,"tag_id3v23_with_wmp_rating",  "Tests ID3v2.3 tag (with WMP rating)", TEST_ENABLED };
static const SDLTest_TestCaseReference id3v23Test7 =
        { (SDLTest_TestCaseFp)tag_id3v23_unicode_tags,"tag_id3v23_unicode_tags",  "Tests ID3v2.3 tag (Unicode tags)", TEST_ENABLED };

static const SDLTest_TestCaseReference id3v24Test1 =
        { (SDLTest_TestCaseFp)tag_id3v24,"tag_id3v24",  "Tests ID3v2.4 tag", TEST_ENABLED };


static const SDLTest_TestCaseReference ly3Test1 =
        { (SDLTest_TestCaseFp)tag_lyrics3,"tag_lyrics3",  "Tests Lyrics3v1 tag", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test2 =
        { (SDLTest_TestCaseFp)tag_lyrics3_with_id3v1,"tag_lyrics3_with_id3v1",  "Tests Lyrics3v1 tag (with ID3v1)", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test3 =
        { (SDLTest_TestCaseFp)tag_lyrics3_with_id3v1_invalid_file,"tag_lyrics3_with_id3v1_invalid_file",  "Tests Lyrics3v1 tag (with ID3v1), invalid file test", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test4 =
        { (SDLTest_TestCaseFp)tag_lyrics3_with_id3v1_invalid_tail,"tag_lyrics3_with_id3v1_invalid_tail",  "Tests Lyrics3v1 tag (with ID3v1), invalid tail", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test5 =
        { (SDLTest_TestCaseFp)tag_lyrics3v2,"tag_lyrics3v2",  "Tests Lyrics3v2 tag", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test6 =
        { (SDLTest_TestCaseFp)tag_lyrics3v2_with_id3v1,"tag_lyrics3v2_with_id3v1",  "Tests Lyrics3v2 tag and ID3v1", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test7 =
        { (SDLTest_TestCaseFp)tag_lyrics3v2_with_id3v1_invalid_file,"tag_lyrics3v2_with_id3v1_invalid_file",  "Tests Lyrics3v2 tag and ID3v1, invalid file", TEST_ENABLED };
static const SDLTest_TestCaseReference ly3Test8 =
        { (SDLTest_TestCaseFp)tag_lyrics3v2_with_id3v1_invalid_tail,"tag_lyrics3v2_with_id3v1_invalid_tail",  "Tests Lyrics3v2 tag and ID3v1, invalid tail", TEST_ENABLED };


static const SDLTest_TestCaseReference mmTest1 =
        { (SDLTest_TestCaseFp)tag_musicmatch,"tag_musicmatch",  "Tests MusicMatch tag", TEST_ENABLED };
static const SDLTest_TestCaseReference mmTest2 =
        { (SDLTest_TestCaseFp)tag_musicmatch_with_id3v1,"tag_musicmatch_with_id3v2",  "Tests MusicMatch tag with ID3v1", TEST_ENABLED };
static const SDLTest_TestCaseReference mmTest3 =
        { (SDLTest_TestCaseFp)tag_musicmatch_with_id3v1_and_2,"tag_musicmatch_with_id3v1_and_2",  "Tests MusicMatch tag with ID3v1 and ID3v2", TEST_ENABLED };

static const SDLTest_TestCaseReference *mp3Tests[] =  {
    &noTagsTest,
    &apeTest1, &apeTest2, &apeTest3, &apeTest4, &apeTest5, &apeTest6, &apeTest7, &apeTest8,
    &id3v1Test1,
    &id3v22Test1, &id3v22Test2,
    &id3v23Test1, &id3v23Test2, &id3v23Test3, &id3v23Test4, &id3v23Test5, &id3v23Test6, &id3v23Test7,
    &id3v24Test1,
    &ly3Test1, &ly3Test2, &ly3Test3, &ly3Test4, &ly3Test5, &ly3Test6, &ly3Test7, &ly3Test8,
    &mmTest1, &mmTest2, &mmTest3,
    NULL
};

/* Rect test suite (global) */
SDLTest_TestSuiteReference mp3utilsTestSuite = {
    "mp3utils",
    NULL,
    mp3Tests,
    NULL
};


/* All test suites */
SDLTest_TestSuiteReference *testSuites[] =  {
    &mp3utilsTestSuite,
    NULL
};

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
    exit(rc);
}

int
main(int argc, char *argv[])
{
    int result;
    (void)argc; (void)argv;

    /* Call Harness */
    result = SDLTest_RunSuites(testSuites, NULL, 0, NULL, 1);

    /* Shutdown everything */
    quit(result);
    return(result);
}
