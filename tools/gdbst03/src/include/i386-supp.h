/****************************************************************************
 *
 *  i386-supp.h
 *
 *  Description:  Data definitions and constants for low level
 *                GDB stub support.
 * 
 *  Terms of use:  This software is provided for use under the terms
 *                 and conditions of the GNU General Public License.
 *                 You should have received a copy of the GNU General
 *                 Public License along with this program; if not, write
 *                 to the Free Software Foundation, Inc., 59 Temple Place
 *                 Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Credits:      Created by Jonathan Brogdon
 *
 *  History
 *  Engineer:           Date:              Notes:
 *  ---------           -----              ------
 *  Jonathan Brogdon    20000629           Genesis
 *  Gordon Schumacher   20020212           Updated for modularity 
 *
 ****************************************************************************/
#ifndef _GDBSUPP_H_
#define _GDBSUPP_H_

extern int putDebugChar(char c);
extern int getDebugChar(void);

#endif /* _GDBSUPP_H_ */
