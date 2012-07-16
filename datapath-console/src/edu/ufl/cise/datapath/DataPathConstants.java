/**
 * DataPathConstants.java
 *
 * @author Praveen Salitra (praveen@cise.ufl.edu)
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
package edu.ufl.cise.datapath;

public final class DataPathConstants
{
	public static final String DATAPATH_DB           = "jdbc:sqlite:/home/datapath/datapath/datapath.sqlite";
	public static final String DATAPATH_GLAS = "/home/datapath/datapath/GLAs/";
	public static final String DATAPATH_PIGGYS = "/home/datapath/datapath/Piggys/";
	public static final String DATAPATH_RUN_PIGGY = "/home/datapath/datapath/runPiggy.sh";
	public static final int    STATUS_OK             = 200;
	public static final int    INTERNAL_SERVER_ERROR = 500;
	public static final int    NO_TYPE_FOUND         = 2004;
	public static final int    TYPE_ALREADY_EXISTS   = 2005;
}
