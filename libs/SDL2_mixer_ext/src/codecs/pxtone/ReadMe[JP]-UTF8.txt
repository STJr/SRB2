////////////////////////////////////////////////////////
//                                                    //
// pxtone play sample (バージョン '22/09/10a)         //
//                                                    //
// - "pxtnDescripter" は無くなりました。              //
//                                                    //
//                                                    //
////////////////////////////////////////////////////////

＜これは何？＞

    ピストンコラージュ（ピスコラ）で作った曲ファイルを
    Windows で再生するだけのサンプルソースコードです。

＜pxtone＞

    ピスコラで作成した曲データから"波形データ"を作ることができる。
    この"波形データ"を wavファイルとして保存したり、音声バッファへ出力してゲームなどの
    BGMにしたりできます。

    ptNoise で作る音声ファイルをゲームの効果音として利用することもできますが、
    このサンプルでは使用していません。

＜予備知識＞

    XAudio2   : Windows で音声を再生するのに使用するAPI。
                XAudio2 を利用するには DirectX SDK が必要。

    Ogg Vorbis: Xiph.orgが開発したフリーの音声ファイルフォーマット。
                音声ファイルに ogg を利用している曲を再生するにはこれが必要。
                Xiph.org で入手できる。(oggを使わない場合は不要)

                Ogg Vorbis を利用する場合は pxtn.h にある "#define pxINCLUDE_OGGVORBIS 1" の
                コメントアウトを外してください。

＜ピスコラの曲を再生する流れ＞

    主に pxtnServiceクラス を利用します。

    １、初期化
      ↓
    ２、ファイル読み込み
      ↓
    ３、再生準備
      ↓
    ４、波形データの生成
      ↓
    ５、音声バッファへ出力

    BGMとして再生する場合は、一度にサンプリングするのではなく
    ４・５を繰り返して少しずつ音声バッファへ出力します。

＜ライセンス的なこと＞

    ・再生に必要な ソースコード(pxtoneフォルダ内)は無償で使えます。改変もOKです。
    ・特に許可をとる必要はありません。
    ・利用の明記についてはお任せします。
    ・利用が原因で何か問題があった場合の責任は負いかねます。

＜ogg/vorbis を公式からダウンロードして使う方法＞

    http://www.vorbis.com/ の FOR DEVELOPERS の downloads より
    libogg-???.zip
    libvorbis-???.zip をダウンロードして展開。

    それらのヘッダを含むフォルダをインクルードにを追加、
    以下のソースコードを含めてビルドします。
    ・libogg¥src¥bitwise.c
    ・libogg¥src¥framing.c
    ・libvorbis¥lib¥analysis.c
    ・libvorbis¥lib¥bitrate.c
    ・libvorbis¥lib¥block.c
    ・libvorbis¥lib¥codebook.c
    ・libvorbis¥lib¥envelope.c
    ・libvorbis¥lib¥floor0.c
    ・libvorbis¥lib¥floor1.c
    ・libvorbis¥lib¥info.c
    ・libvorbis¥lib¥lookup.c
    ・libvorbis¥lib¥lpc.c
    ・libvorbis¥lib¥lsp.c
    ・libvorbis¥lib¥mapping0.c
    ・libvorbis¥lib¥mdct.c
    ・libvorbis¥lib¥psy.c
    ・libvorbis¥lib¥registry.c
    ・libvorbis¥lib¥res0.c
    ・libvorbis¥lib¥sharedbook.c
    ・libvorbis¥lib¥smallft.c
    ・libvorbis¥lib¥synthesis.c
    ・libvorbis¥lib¥vorbisenc.c
    ・libvorbis¥lib¥vorbisfile.c
    ・libvorbis¥lib¥window.c
    
    以下は除外する。
    ・libvorbis¥lib¥barkmel.c
    ・libvorbis¥lib¥psytune.c
    ・libvorbis¥lib¥tone.c
