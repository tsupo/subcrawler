/*
 * freshreader �p �N���[���[�N���c�[��
 *      by H.Tsujimura (tsupo@na.rim.or.jp)     30 Jan 2006
 *
 * $Log: /comm/subcrawler/subcrawler.c $
 * 
 * 1     09/05/14 3:53 tsupo
 * (1) �r���h���̃f�B���N�g���\���𐮗�
 * (2) VSS�T�[�o���_��ύX
 * 
 * 6     06/10/23 16:32 Tsujimura543
 * ��M�o�b�t�@�I�[�o�[�����΍��ǉ�
 * 
 * 5     06/02/06 20:50 Tsujimura543
 * FreshReader ��2 �Ή��֘A�̏C�������{ (���g���C�����̒ǉ��A�s�v�� http
 * ���N�G�X�g�̍팸)
 * 
 * 4     06/02/06 11:33 Tsujimura543
 * subcrowler �� subcrawler �ɒu��
 * 
 * 3     06/02/03 20:19 Tsujimura543
 * FreshReader ��2 �Ŕ����� login ���̉������ς�����̂ɑΉ�
 * (���o�[�W�����ł������悤�ɍl���ς�)
 * 
 * 2     06/01/30 19:54 Tsujimura543
 * �\�[�X�𐮌`
 * 
 * 1     06/01/30 19:50 Tsujimura543
 * �ŏ��̔�
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
             "�g����:\n"
             "  subcrawler freshreader���[�gURL "
             "[-u ���[�U��] [-p �p�X���[�h] [-s]\n"
             "\t-s: �T�C�����g���[�h(�r���o�߂�\�����Ȃ�)\n" );
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

    /* ���O�C�� (+ �E�t���[���擾) */
    memset( cookie, 0x00, MAX_COOKIE_LEN );
    sprintf( url, "%sindex.php", rootURL );
    sprintf( request,
             "userid=%s&"
             "password=%s&"
             "keeplogin=1&"
             "submit=%s&"
             "logintarget=",
             username, password, encodeURL(sjis2utf("���O�C��")) );
    setUpReceiveBuffer( response, sz );
    http_postEx( url, "application/x-www-form-urlencoded", request, response, cookie );
    if ( !(*response) && !(*cookie) ) {
        fprintf( stderr, "���O�C���Ɏ��s���܂���\n" );
        goto EXIT;
    }

#if 0
    /* ���t���[���擾 */
    sprintf( url, "%sfeedlist.php", rootURL );
    setTargetURL( url );
    setUpReceiveBuffer( response, sz );
    http_getEx( url, response, cookie );
    if ( !(*response) ) {
        fprintf( stderr, "�G���[���������܂����B�����𒆒f���܂�\n" );
        goto EXIT;
    }
#endif

    /* �N���[���J�n�w�����s */
    // �v��: http://www.example.com/freshreader/subcrawler.php?of=-1&d=1138605567781
    // ����: <continue param="453;0;"/>
    // �v��: http://www.example.com/freshreader/subcrawler.php?of=0&d=1138605568609
    //  �c�c
    // ����: <continue param="453;XXX;"/>
    // �v��: http://www.example.com/freshreader/subcrawler.php?of=XXX&d=���ݎ���
    //  �c�c
    // ����: <continue param="453;444;"/>
    // �v��: http://www.example.com/freshreader/subcrawler.php?of=444&d=1138605989562
    // ����: <finished param="453;0;"/>
    // �v��: http://www.example.com/freshreader/feedlist.php?a=fin&t=453&d=1138605989562
    // ����: (��)
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
                         "�S %d �T�C�g�̂��� %d �T�C�g�̐V��������M����\n",
                                 total, offset );
                    else
                        fprintf( stderr,
                                 "�S %d �T�C�g���̐V��������M�J�n\n",
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
                fprintf( stderr, "%d �T�C�g���̐V������S�Ď�M����\n",
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
        fprintf( stderr, "freshreader �̃��[�g URL ���w�肵�Ă�������" );
        return ( 255 );
    }
    if ( strncmp( rootURL, "http", 4 ) != 0 ) {
        fprintf( stderr, "������ URL ���w�肵�Ă�������" );
        return ( 255 );
    }
    if ( rootURL[strlen(rootURL) - 1] != '/' )
        strcat( rootURL, "/" );

    if ( username[0] == NUL )
        inputString( username,
                     "���[�U��: ", FALSE );

    if ( password[0] == NUL )
        inputString( password,
                     "�p�X���[�h: ", TRUE );

    setUseProxy( useProxy );
    subcrawler( rootURL, username, password, verbose );

    return ( 1 );
}


