#include <freekick.h>

int fk_cmd_hset(fk_conn *conn)
{
	int rt;
	fk_dict *dct;
	fk_item *hkey, *key;
	fk_item *hitm, *itm;

	hkey = fk_conn_arg_get(conn, 1);
	key = fk_conn_arg_get(conn, 2);
	hitm = fk_dict_get(server.db[conn->db_idx], hkey);
	if (hitm == NULL) {
		dct = fk_dict_create(&db_dict_eop);
		itm = fk_conn_arg_get(conn, 3);
		fk_dict_add(dct, key, itm);
		hitm = fk_item_create(FK_ITEM_DICT, dct);/* do not increase ref for local var */
		fk_dict_add(server.db[conn->db_idx], hkey, hitm);
		rt = fk_conn_int_rsp_add(conn, 1);
		if (rt == FK_CONN_ERR) {
			return FK_ERR;
		}
		return FK_OK;
	}

	if (fk_item_type(hitm) == FK_ITEM_DICT) {
		dct = (fk_dict *)fk_item_raw(hitm);
		itm = fk_conn_arg_get(conn, 3);
		fk_dict_add(dct, key, itm);
		rt = fk_conn_int_rsp_add(conn, 1);
		if (rt == FK_CONN_ERR) {
			return FK_ERR;
		}
		return FK_OK;
	}

	rt = fk_conn_error_rsp_add(conn, FK_RSP_TYPE_ERR, sizeof(FK_RSP_TYPE_ERR) - 1);
	if (rt == FK_CONN_ERR) {
		return FK_ERR;
	}
	return FK_OK;
}

int fk_cmd_hget(fk_conn *conn)
{
	int rt;
	fk_dict *dct;
	fk_item *hitm, *itm;
	fk_str *hkey, *key, *value;

	hkey = fk_conn_arg_get(conn, 1);
	key = fk_conn_arg_get(conn, 2);
	hitm = fk_dict_get(server.db[conn->db_idx], hkey);
	if (hitm == NULL) {
		rt = fk_conn_bulk_rsp_add(conn, FK_RSP_NIL);
		if (rt == FK_CONN_ERR) {
			return FK_ERR;
		}
		return FK_OK;
	} 

	if (fk_item_type(hitm) != FK_ITEM_DICT) {
		rt = fk_conn_error_rsp_add(conn, FK_RSP_TYPE_ERR, sizeof(FK_RSP_TYPE_ERR) - 1);
		if (rt == FK_CONN_ERR) {
			return FK_ERR;
		}
		return FK_OK;
	} 

	dct = (fk_dict *)fk_item_raw(hitm);
	itm = fk_dict_get(dct, key);
	if (itm == NULL) {
		rt = fk_conn_bulk_rsp_add(conn, FK_RSP_NIL);
		if (rt == FK_CONN_ERR) {
			return FK_ERR;
		}
		return FK_OK;
	} 

	if (itm->type != FK_ITEM_STR) {
		rt = fk_conn_error_rsp_add(conn, FK_RSP_TYPE_ERR, sizeof(FK_RSP_TYPE_ERR) - 1);
		if (rt == FK_CONN_ERR) {
			return FK_ERR;
		}
		return FK_OK;
	}

	value = (fk_str *)fk_item_raw(itm);
	rt = fk_conn_bulk_rsp_add(conn, (int)(fk_str_len(value)));
	if (rt == FK_CONN_ERR) {
		return FK_ERR;
	}
	rt = fk_conn_content_rsp_add(conn, fk_str_raw(value), fk_str_len(value));
	if (rt == FK_CONN_ERR) {
		return FK_ERR;
	}

	return FK_OK;
}
