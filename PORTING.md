## Imporant symbols in uVisor

<table>
  <tbody>
    <tr>
      <th>Symbol name</th>
      <th>Description</th>
      <th>ARMv7M MPU</th>
      <th>K64F MPU</th>
    </tr>
    <tr>
      <td>__stack_start__</td>
      <td>start of the protected uVisor stack segment</td>
      <td>aligned to the size of the stack</td>
      <td>aligned to 32 bytes</td>
    </tr>
    <tr>
      <td>__stack_top__</td>
      <td>stack pointer to the privileged uVisor stack</td>
      <td>aligned to the <b>size_of_stack/8</b> </td>
      <td>aligned to 32 bytes</td>
    </tr>
    <tr>
      <td>__stack_end__</td>
      <td>points to the end of the privileged uViisor stack</td>
      <td>stack needs to be 2^n size > 256</td>
      <td>aligned to 32 bytes - ensure space for two protection bands on top and bottom, each eat least 32 bytes</td>
    </tr>
  </tbody>
</table>
