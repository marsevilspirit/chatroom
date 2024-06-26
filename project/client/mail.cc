#include "client_menu.h"


#define FAIL	-1
#define OK	     0

#define SUBJECT 	"chatroom验证码"  					/* 邮件主题*/
#define SERVER 		"smtp.163.com:25"			/* 邮件服务器地址 */
char USERNAME[] = "15643870618@163.com"; // 使用字符数组定义邮箱账号
char PASSWOED[] = "HPYMJHDLRSHOTECT"; // 使用字符数组定义邮箱密码

/* 回调函数 设置账号密码 */
static int authinteract(auth_client_request_t request, char **result, int fields, void *args)
{
	int	i;

	for (i=0; i < fields; i++)
	{
		if (request[i].flags & AUTH_PASS)
			result[i] = PASSWOED; 
		else	
			result[i] = USERNAME;
	}
	return 1;
}

/* 回调函数 打印客户端与服务器交互情况 可以不要 */
static void monitor_cb(const char *buf, int buflen, int writing, void *arg)
{
	int i;
	
	if (writing == SMTP_CB_HEADERS)
		printf("H: ");
	else if(writing)
		printf("C: ");
	else
		printf("S: ");

	const char *p = buf;
	for(i=0; p[i]!='\n'; i++)
	{
		printf("%c", p[i]);
	}
	printf("\n");
}

int verify(const char* usermail, char* verify_number)
{
	int	ret;
	FILE *tmp_fp = NULL;
	smtp_session_t		session;
	smtp_message_t		message;
	smtp_recipient_t	recipient;
	auth_context_t		authctx;
	const smtp_status_t *status;
	
	auth_client_init();
	session = smtp_create_session();
	message = smtp_add_message(session);
	
	smtp_set_monitorcb(session, monitor_cb, stdout, 1);


	/* 设置邮件主题 */
	smtp_set_header(message, "Subject",SUBJECT);
	smtp_set_header_option(message, "Subject", Hdr_OVERRIDE, 1);
	
	/* 设置收件人 */
	smtp_set_header(message, "To", NULL, usermail);
	
	/* 设置发件人 */
	smtp_set_reverse_path(message, USERNAME);
	
	/* 添加收件人 */
	recipient = smtp_add_recipient(message, usermail);

	/* 设置服务器域名和端口 */
	smtp_set_server(session, SERVER);
	
	/* 设置邮件正文 */
	tmp_fp = tmpfile();
	if (tmp_fp == NULL)
	{
		printf("create temp file failed: %s\n", strerror(errno));
		return FAIL;
	}
	fprintf(tmp_fp, "\r\n\t\t%s",verify_number);
	rewind(tmp_fp);
	smtp_set_message_fp(message, tmp_fp);
	
	/* 设置登录验证相关的东西 */
	authctx = auth_create_context();
	if (authctx == NULL)
	{
		return FAIL;
	}

	auth_set_mechanism_flags(authctx, AUTH_PLUGIN_PLAIN, 0);
	auth_set_interact_cb(authctx, authinteract, NULL);
	smtp_auth_set_context(session, authctx);
	
	if (!smtp_start_session(session)) 
	{
		char buf[128];
		printf( "SMTP server problem %s", smtp_strerror(smtp_errno(), buf, sizeof buf));
		ret = FAIL;
	}
	else 
	{
		/* 输出邮件发送情况 如果 status->code== 250则表明发送成功 */
		status = smtp_message_transfer_status(message);
		printf("%d %s", status->code, (status->text != NULL) ? status->text : "\n");
		ret = OK;
	}

	/* 释放资源 */
	smtp_destroy_session(session);
	if (authctx)
		auth_destroy_context(authctx);
	auth_client_exit();
	fclose(tmp_fp);

    system("clear");
	return ret;		
}

