/* RSA dummy - thunk function */
void thunk(void)
{
    __SVC0(8);
}

uint32_t f_rsa(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    dprintf(" --- RSA ---\n\r");
    return 0x0;
}

uint32_t f_f1(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    dprintf(" --- f1  ---\n\r");
    return 0x1;
}

uint32_t f_f2(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    dprintf(" --- f2  ---\n\r");
    return 0x2;
}

uint32_t start(uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    return __SVC3(7, a0, (uint32_t) rsa_dummy, (uint32_t) thunk);
}
