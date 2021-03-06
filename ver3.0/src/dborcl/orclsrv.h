/*
 * The Software License
 * =================================================================================
 * Copyright (c) 2003-2010 The Terimber Corporation. All rights reserved.
 * =================================================================================
 * Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution.
 * The end-user documentation included with the redistribution, if any, 
 * must include the following acknowledgment:
 * "This product includes software developed by the Terimber Corporation."
 * =================================================================================
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
 * IN NO EVENT SHALL THE TERIMBER CORPORATION OR ITS CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ================================================================================
*/

#ifndef _terimber_orclsrv_h_
#define _terimber_orclsrv_h_

#include "db/db.h"
#include "dborcl/orclinc.h"

BEGIN_TERIMBER_NAMESPACE
#pragma pack(4)

//! \class orcl_dbserver
//! \brief class implements interface dbserver
class orcl_dbserver : public dbserver_impl
{
public:
	//! \brief constructor
	orcl_dbserver(	size_t ident							//!< server ident
					);
	//! \brief destructor
	virtual 
	~orcl_dbserver();

protected:
	// database specific
	virtual void v_connect(bool trusted_connection, const char* connection_string);
	virtual void v_disconnect();
	virtual void v_start_transaction();
	virtual void v_commit();
	virtual void v_rollback();
	virtual bool v_is_connect_alive();
	virtual void v_before_execute();
	virtual void v_after_execute();
	virtual void v_before_bind_columns();
	virtual void v_execute();
	virtual void v_close();
	virtual void v_fetch();
	virtual void v_replace_quote();
	virtual void v_bind_one_param(size_t index);
	virtual void v_bind_one_column(size_t index);
	virtual size_t v_get_number_columns();
	virtual void v_convert_one_value(size_t row, size_t index, terimber_db_value& val);
	virtual void v_get_one_column_info(size_t index);
	virtual void v_form_sql_string();
	virtual void v_rebind_one_param(size_t index);
	virtual void v_interrupt_async();
	virtual dbtypes v_native_type_to_client_type(size_t native_type);

private:
	OCIEnv*			_envhp;									//!< oracle environment handle
	OCISvcCtx*		_svchp;									//!< oracle server context handle
	OCIError*		_errhp;									//!< oracle error interface handle
	OCIStmt*		_stmthp;								//!< oracle SQL statement handle
};

/////////////////////////////////////////////////////////////////////////
// pool support
//! \class orcl_db_creator
//! \brief db creator
class orcl_db_creator : public db_creator< orcl_db_creator > 
{
public:
	//! \brief creates object
	static
	db_entry* 
	create(			const db_arg& arg						
					);
};

typedef pool< orcl_db_creator > orcl_db_pool_t;

#pragma pack()
END_TERIMBER_NAMESPACE

#endif // _terimber_orclsrv_h_

