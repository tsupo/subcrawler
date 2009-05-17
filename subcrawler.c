/*
 * freshreader 用 クローラー起動ツール
 *      by H.Tsujimura (tsupo@na.rim.or.jp)     30 Jan 2006
 *
 * $Log: /comm/subcrawler/subcrawler.c $
 * 
 * 1     09/05/14 3:53 tsupo
 * (1) ビルド環境のディレクトリ構造を整理
 * (2) VSSサーバ拠点を変更
 * 
 * 6     06/10/23 16:32 Tsujimura543
 * 受信バッファオーバーラン対策を追加
 * 
 * 5     06/02/06 20:50 Tsujimura543
 * FreshReader β2 対応関連の修正を実施 (リトライ処理の追加、不要な http
 * リクエストの削減)
 * 
 * 4     06/02/06 11:33 Tsujimura543
 * subcrowler → subcrawler に置換
 * 
 * 3     06/02/03 20:19 Tsujimura543
 * FreshReader β2 で微妙に login 時の応答が変わったのに対応
 * (旧バージョンでも動くように考慮済み)
 * 
 * 2     06/01/30 19:54 Tsujimura543
 * ソースを整形
 * 
 * 1     06/01/30 19:50 Tsujimura543
 * 最初の版
 */

#include <stdlib.h>
#include <time.h>
#include "xmlRPC.h"

#ifndef	lint
static char	*rcs_id =
"$Header: /comm/subcrawler/subcrawler.c 1     09/05/14 3:53 tsupo $";
#endif

void
usage( void )
{
    fprintf( stderr,
             "使い方:\n"
             "  subcrawler freshreaderルートURL "
             "[-u ユーザ名] [-p パスワード] [-s]\n"
             "\t-s: サイレントモード(途中経過を表示しない)\n" );
}

void
subcrawler(
        const char *rootURL,
        const char *username,
        const char *password,
        int        verbose
    )
{
    char        url[MAX_URLLENGTH];
    char        *request;
    char        *response;
    char        *cookie;
    time_t      t;
    SYSTEMTIME  systime;
    double      s;
    char        *p, *q;
    int         offset;
    int         total;
    int         retry = 0;
    size_t      sz = MAX_CONTENT_SIZE * 4;

    request = (char *)malloc( BUFSIZ * 4 );
    if ( !request )
        return;
    response = (char *)malloc( MAX_CONTENT_SIZE * 4 );
    if ( !response ) {
        free( request );
        return;
    }
    cookie = (char *)malloc( MAX_COOKIE_LEN );
    if ( !cookie ) {
        free( response );
        free( request );
        return;
    }

    /* ログイン (+ 右フレーム取得) */
    memset( cookie, 0x00, MAX_COOKIE_LEN );
    sprintf( url, "%sindex.php", rootURL );
    sprintf( request,
             "userid=%s&"
             "password=%s&"
             "keeplogin=1&"
             "submit=%s&"
             "logintarget=",
             username, password, encodeURL(sjis2utf("ログイン")) );
    setUpReceiveBuffer( response, sz );
    http_postEx( url, "application/x-www-form-urlencoded", request, response, cookie );
    if ( !(*response) && !(*cookie) ) {
        fprintf( stderr, "ログインに失敗しました\n" );
        goto EXIT;
    }

#if 0
    /* 左フレーム取得 */
    sprintf( url, "%sfeedlist.php", rootURL );
    setTargetURL( url );
    setUpReceiveBuffer( response, sz );
    http_getEx( url, response, cookie );
    if ( !(*response) ) {
        fprintf( stderr, "エラーが発生しました。処理を中断します\n" );
        goto EXIT;
    }
#endif

    /* クロール開始指示発行 */
    // 要求: http://www.example.com/freshreader/subcrawler.php?of=-1&d=1138605567781
    // 応答: <continue param="453;0;"/>
    // 要求: http://www.example.com/freshreader/subcrawler.php?of=0&d=1138605568609
    //  ……
    // 応答: <continue param="453;XXX;"/>
    // 要求: http://www.example.com/freshreader/subcrawler.php?of=XXX&d=現在時刻
    //  ……
    // 応答: <continue param="453;444;"/>
    // 要求: http://www.example.com/freshreader/subcrawler.php?of=444&d=1138605989562
    // 応答: <finished param="453;0;"/>
    // 要求: http://www.example.com/freshreader/feedlist.php?a=fin&t=453&d=1138605989562
    // 応答: (略)
    t = time( &t );
    GetSystemTime( &systime );
    s = t * 1000.0 + systime.wMilliseconds;
    sprintf( url, "%ssubcrawler.php?of=-1&d=%.f", rootURL, s );
    setTargetURL( url );
    setUpReceiveBuffer( response, sz );
    http_getEx( url, response, cookie );
    while ( *response ) {
        if ( (p = strstr( response, "<continue param=\"" )) != NULL ) {
            p += 17;
            total = atol( p );
            q = strchr( p, ';' );
            if ( q ) {
                q++;
                offset = atol( q );
                if ( verbose ) {
                    if ( offset > 0 )
                        fprintf( stderr,
                         "全 %d サイトのうち %d サイトの新着情報を受信完了\n",
                                 total, offset );
                    else
                        fprintf( stderr,
                                 "全 %d サイト分の新着情報を受信開始\n",
                                 total );
                }

                retry = 0;
                do {
                    t = time( &t );
                    GetSystemTime( &systime );
                    s = t * 1000.0 + systime.wMilliseconds;
                    sprintf( url, "%ssubcrawler.php?of=%d&d=%.f",
                             rootURL, offset, s );
                    setTargetURL( url );
                    setUpReceiveBuffer( response, sz );
                    http_getEx( url, response, cookie );
                    if ( ++retry >= 3 )
                        break;
                } while ( !(*response) );
                continue;
            }
        }
        else if ( (p = strstr( response, "<finished param=\"" )) != NULL ) {
            p += 17;
            offset = atol( p );
            if ( verbose )
                fprintf( stderr, "%d サイト分の新着情報を全て受信完了\n",
                         offset );
            t = time( &t );
            GetSystemTime( &systime );
            s = t * 1000.0 + systime.wMilliseconds;
            sprintf( url, "%sfeedlist.php?a=fin&t=%d&d=%.f",
                     rootURL, offset, s );
            setUpReceiveBuffer( response, sz );
            setTargetURL( url );
            http_getEx( url, response, cookie );
            continue;
        }

        break;
    }

  EXIT:
    free( cookie );
    free( response );
    free( request );
}

int
main( int argc, char *argv[] )
{
    char    username[80];
    char    password[80];
    char    rootURL[384];
    int     i;
    int     useProxy = FALSE;
    int     verbose  = TRUE;

    memset( username, 0x00, 80 );
    memset( password, 0x00, 80 );
    memset( rootURL,  0x00, 384 );

    useProxy = isUsedProxy();

    if ( argc <= 1 ) {
        usage();
        return ( 254 );
    }

    for ( i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'u':
                if ( argv[i][2] )
                    strcpy( username, &argv[i][2] );
                else if ( i + 1 < argc )
                    strcpy( username, argv[++i] );
                break;
            case 'p':
                if ( argv[i][2] )
                    strcpy( password, &argv[i][2] );
                else if ( i + 1 < argc )
                    strcpy( password, argv[++i] );
                break;
            case 's':
                verbose = FALSE;
                break;
            }

            continue;
        }
        else
            strcpy( rootURL, argv[i] );
    }

    if ( rootURL[0] == NUL ) {
        fprintf( stderr, "freshreader のルート URL を指定してください" );
        return ( 255 );
    }
    if ( strncmp( rootURL, "http", 4 ) != 0 ) {
        fprintf( stderr, "正しい URL を指定してください" );
        return ( 255 );
    }
    if ( rootURL[strlen(rootURL) - 1] != '/' )
        strcat( rootURL, "/" );

    if ( username[0] == NUL )
        inputString( username,
                     "ユーザ名: ", FALSE );

    if ( password[0] == NUL )
        inputString( password,
                     "パスワード: ", TRUE );

    setUseProxy( useProxy );
    subcrawler( rootURL, username, password, verbose );

    return ( 1 );
}


